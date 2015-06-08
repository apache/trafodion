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

using System.Collections.Generic;
using System.Data.Odbc;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// 
    /// </summary>
    public class TrafodionIndex : PartitionedSchemaObject
    {
        #region Member Variables

        private List<TrafodionIndexColumn> _sqlMxIndexColumns = null;

        // This field is permanently bound to the index. It can never ever change.
        private readonly IndexedSchemaObject _indexedSchemaObject;

        private bool _areAttributesLoaded = false;

        // Attribute Variables
        private string _isAuditCompress;
        private string _isBuffered;
        private string _isDataCompressed;
        private string _isIndexCompressed;
        private string _isClearOnPurge;
        private long _theBlockSize;
        private int _theKeyLength;
        private int _theLockLength;
        private string _isUniques;
        private string _isExplicit;
        private string _thePartitioningScheme;
        private string _isAudited;
        private string _isValidData;
        private string _theRowFormat = "";

        // Column Variables
        private bool _isAscending = false;
        private bool _theSystemAddedColumn = false;

        /// <summary>
        /// The Namespace of an Index
        /// </summary>
        public const string ObjectNameSpace = "IX";

        /// <summary>
        /// The Object type of an Index
        /// </summary>
        public const string ObjectType = "IX";
        private string[] ddlPatterns = { "CREATE UNIQUE INDEX", "CREATE INDEX" };


        #endregion
        
        /// <summary>
        /// Constructor for TrafodionIndex
        /// </summary>
        /// <param name="aIndexedSchemaObject">the Schema Object which contains indexes</param>
        /// <param name="anInternalName"> the Internal Name</param>
        /// <param name="aUID">the UID of this Index</param>
        /// <param name="aCreateTime">the Time when this Index was created</param>
        /// <param name="aRedefTime">the Time when this Index was redefined</param>
        /// <param name="aSecurityClass">the security class</param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionIndex(IndexedSchemaObject aIndexedSchemaObject, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aIndexedSchemaObject.TheTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
            _indexedSchemaObject = aIndexedSchemaObject;
            base.IsIndex = true;
        }


        /// <summary>
        /// Returns the schema object type
        /// </summary>
        override public string SchemaObjectType
        {
            get
            {
                return "IX";
            }
        }

        /// <summary>
        /// Displayable Object type for the index
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.Index;
            }
        }

        /// <summary>
        /// Returns the schema object name space
        /// </summary>
        override public string SchemaObjectNameSpace
        {
            get
            {
                return "IX";
            }
        }

        /// <summary>
        /// Gets the list of column definitions
        /// </summary>
        public List<TrafodionIndexColumn> TrafodionColumns
        {
            get
            {
                if (_sqlMxIndexColumns == null)
                {
                    _sqlMxIndexColumns = new TrafodionIndexColumnsLoader().Load(this);
                }
                return _sqlMxIndexColumns;
            }

        }

        /// <summary>
        /// Load the attributes of the index
        /// </summary>
        /// <returns></returns>
        public bool LoadAttributes()
        {
            if (_areAttributesLoaded)
            {
                return true;
            }

            Connection theConnection = null;

            //try
            //{
            //    theConnection = GetConnection();

            //    OdbcDataReader theReader = Queries.ExecuteSelectIndexAttributes(theConnection, TheTrafodionCatalog, this, TheTrafodionSchema.Version);
            //    theReader.Read();
            //    SetAttributes(theReader);                
            //}
            //finally
            //{
            //    if (theConnection != null)
            //    {
            //        theConnection.Close();
            //    }
            //}
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
            _isBuffered = aReader.GetString(1).Trim();
            _isDataCompressed = aReader.GetString(2).Trim();
            _isIndexCompressed = aReader.GetString(3).Trim();
            _isClearOnPurge = aReader.GetString(4).Trim();
            _theBlockSize = aReader.GetInt64(5);
            _theKeyLength = aReader.GetInt16(6);
            _theLockLength = aReader.GetInt16(7);
            _isUniques = aReader.GetString(9).Trim();
            _isExplicit = aReader.GetString(10).Trim();
            _thePartitioningScheme = aReader.GetString(11).Trim();
            _isAudited = aReader.GetString(12).Trim();
            _isValidData = aReader.GetString(13).Trim();
            _theRowFormat = aReader.GetString(14).Trim();
        }

        /// <summary>
        /// Resets the index model
        /// </summary>
        override public void Refresh()
        {
            //Create a temp model
            TrafodionIndex anIndex = this.IndexedSchemaObject.LoadIndexByName(this.InternalName);

            //If temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (anIndex == null)
            {
                //Both the parent object model and schema model may have reference to this index model
                //So remove the index model from their lists
                this.IndexedSchemaObject.TrafodionIndexes.Remove(this);
                this.IndexedSchemaObject.TheTrafodionSchema.TrafodionIndexes.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(anIndex) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                //Both the parent object model and schema model may have reference to this index model
                //So remove the old index model and add the new index model to their lists
                this.IndexedSchemaObject.TrafodionIndexes.Remove(this);
                this.IndexedSchemaObject.TheTrafodionSchema.TrafodionIndexes.Remove(this);

                this.IndexedSchemaObject.TrafodionIndexes.Add(anIndex);
                this.IndexedSchemaObject.TheTrafodionSchema.TrafodionIndexes.Add(anIndex);
                this.OnModelReplacedEvent(anIndex);
            }
            else
            {
                base.Refresh();

                // Clear attributes
                _areAttributesLoaded = false;
                _isAuditCompress = null;
                _isBuffered = null;
                _isDataCompressed = null;
                _isIndexCompressed = null;
                _isClearOnPurge = null;
                _theBlockSize = 0;
                _theKeyLength = 0;
                _theLockLength = 0;
                _isUniques = null;
                _isExplicit = null;
                _thePartitioningScheme = null;
                _isAudited = null;
                _isValidData = null;
                _theRowFormat = null;

                // Clear other member variables
                _sqlMxIndexColumns = null;
            }
        }

        #region Properties
        
        /// <summary>
        /// Gets the IndexedSchemaObject
        /// </summary>
        public IndexedSchemaObject IndexedSchemaObject
        {
            get { return _indexedSchemaObject; }
        }

        /// <summary>
        /// Indicates if the index is in ascending or descending order
        /// </summary>
        public bool IsAscending
        {
            get { return _isAscending; }
        }

        /// <summary>
        /// Indicates if the column was system added or user added
        /// </summary>
        public bool TheSystemAddedColumn
        {
            get { return _theSystemAddedColumn; }
        }

        /// <summary>
        /// Indicates whether the index is audit compressed
        /// </summary>
        public string IsAuditCompress
        {
            get { LoadAttributes(); return Utilities.OnOff(_isAuditCompress); }
        }

        /// <summary>
        /// Indicates whether the index is buffered
        /// </summary>
        public string IsBuffered
        {
            get { LoadAttributes(); return _isBuffered; }
        }

        /// <summary>
        /// Indicates whether the index is data compressed
        /// </summary>
        public string IsDataCompressed
        {
            get { LoadAttributes(); return _isDataCompressed; }
        }

        /// <summary>
        /// Indicates whether the index is index compressed
        /// </summary>
        public string IsIndexCompressed
        {
            get { LoadAttributes(); return _isIndexCompressed; }
        }

        /// <summary>
        /// Indates whether the index is cleared on a purge
        /// </summary>
        public string IsClearOnPurge
        {
            get { LoadAttributes(); return Utilities.OnOff(_isClearOnPurge); }
        }

        /// <summary>
        /// Returns the block size of the index
        /// </summary>
        public long TheBlockSize
        {
            get { LoadAttributes(); return _theBlockSize; }
        }

        /// <summary>
        /// The key length of the index
        /// </summary>
        public int TheKeyLength
        {
            get { LoadAttributes(); return _theKeyLength; }
        }

        /// <summary>
        /// The lock length of the index
        /// </summary>
        public int TheLockLength
        {
            get { LoadAttributes(); return _theLockLength; }
        }

        /// <summary>
        /// Indicates whether the index is unique
        /// </summary>
        public string IsUniques
        {
            get { LoadAttributes(); return _isUniques; }
        }

        /// <summary>
        /// Indicates whether the index if explicit
        /// </summary>
        public string IsExplicit
        {
            get { LoadAttributes(); return _isExplicit; }
        }

        /// <summary>
        /// Indicates if how the index if partitioned
        /// </summary>
        public string ThePartitioningScheme
        {
            get { LoadAttributes(); return _thePartitioningScheme; }
        }

        /// <summary>
        /// Indicates if the index is audited
        /// </summary>
        public string IsAudited
        {
            get { LoadAttributes(); return _isAudited; }
        }

        /// <summary>
        /// Indicates whether the index has valid data
        /// </summary>
        public string IsValidData
        {
            get { LoadAttributes(); return Utilities.YesNo(_isValidData); }
        }

        /// <summary>
        /// Indicates the row format of the index
        /// </summary>
        public string TheRowFormat
        {
            get { LoadAttributes(); return _theRowFormat; }
        }

        /// <summary>
        /// Returns a string that can be used to display the block size intelligently
        /// </summary>
        public string FormattedBlockSize
        {
            get { return Utilities.FormatSize(TheBlockSize); }
        }

        /// <summary>
        /// Returns a string that can be used to display whether the index if system created or user created
        /// </summary>
        public string IsSystemCreated
        {
            get { return Utilities.YesNo(IsExplicit.Equals("Y") ? "N" : "Y"); }
        }

        /// <summary>
        /// Returns a string that can be used to display if the index is partitioned or not
        /// </summary>
        public string IsHashPartitioned
        {
            get { return (ThePartitioningScheme.Equals("HP") || ThePartitioningScheme.Equals("H2")) ? "Yes" : "No"; }
        }

        /// <summary>
        /// DDL text for the Index
        /// </summary>
        public override string DDLText
        {
            get
            {
                if (_ddlText == null || _ddlText.Length == 0)
                {
                    //Get the DDL text from the referenced object's DDL.
                    _ddlText = TrafodionObjectDDLLoader.FindChildDDL(IndexedSchemaObject.DDLText, ExternalName,
                        ddlPatterns);
                }
                return _ddlText;
            }
        }

        #endregion        

        /// <summary>
        /// This class is used to load column information for an index
        /// </summary>
        class TrafodionIndexColumnsLoader : TrafodionObjectsLoader<TrafodionIndex, TrafodionIndexColumn>
        {

            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionIndex aTrafodionIndex)
            {
                return Queries.ExecuteSelectIndexColumnInfo(aConnection, aTrafodionIndex.TheTrafodionCatalog, aTrafodionIndex, aTrafodionIndex.TheTrafodionSchema.Version);
            }

            /// <summary>
            /// Overrides the load method to add table index columns to the list
            /// </summary>
            /// <param name="aList">the list of TrafodionIndex Columns that is to be loaded with data</param>
            /// <param name="aTrafodionIndex">the TrafodionIndex that is supplying data</param>
            /// <param name="aReader">the Reader that contains the data to be loaded into the list</param>
            override protected void LoadOne(List<TrafodionIndexColumn> aList, TrafodionIndex aTrafodionIndex, OdbcDataReader aReader)
            {
                aList.Add(new TrafodionIndexColumn(aTrafodionIndex, aReader.GetString(0).TrimEnd(),
                        aReader.GetString(1).Trim(),
                        aReader.GetString(2).Trim()));
            }
        }

    }
}
