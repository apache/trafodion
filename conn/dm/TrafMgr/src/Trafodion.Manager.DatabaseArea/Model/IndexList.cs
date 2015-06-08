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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class manages indexes for other objects.
    /// </summary>
    public class IndexList : List<TrafodionIndex>
    {
        #region Fields

        private IndexedSchemaObject _indexedObject;

        #endregion

        /// <summary>
        /// Creates a new indexable delegate.
        /// </summary>
        /// <param name="indexedObject">The object whose indexes this class wraps.</param>
        public IndexList(IndexedSchemaObject indexedObject)
        {
            _indexedObject = indexedObject;
            Refresh();
        }

        /// <summary>
        /// Refreshs the list of indexes.
        /// </summary>
        public void Refresh()
        {
            Clear();
            InsertRange(0, new TrafodionIndexesLoader().Load(_indexedObject));
        }

        /// <summary>
        /// This class is used to load the Indexes of a schema object.
        /// </summary>
        internal class TrafodionIndexesLoader : TrafodionObjectsLoader<IndexedSchemaObject, TrafodionIndex>
        {
            /// <summary>
            /// Returns the ODBC Reader that contains the result of a query to get the list of index names
            /// </summary>
            /// <param name="aConnection">an ODBC Connection</param>
            /// <param name="aIndexedSchemaObject">An indexable object.</param>
            /// <returns>returns an ODBCDataReader with data from the query</returns>
            override protected OdbcDataReader GetQueryReader(Connection aConnection, IndexedSchemaObject aIndexedSchemaObject)
            {
                return Queries.ExecuteSelectIndexNames(
                        aConnection,
                        aIndexedSchemaObject.TheTrafodionSchema.TheTrafodionCatalog,
                        aIndexedSchemaObject.TheTrafodionSchema.InternalName,
                        aIndexedSchemaObject.UID
                    );
            }

            /// <summary>
            /// Overrides the load method to add IndexedSchemaObject indexes to the list
            /// </summary>
            /// <param name="aList">the list of TrafodionIndex that is to be loaded with data</param>
            /// <param name="aIndexedSchemaObject">the object that is supplying data</param>
            /// <param name="aReader">the Reader that contains the data to be loaded into the list</param>
            override protected void LoadOne(List<TrafodionIndex> aList, IndexedSchemaObject aIndexedSchemaObject, OdbcDataReader aReader)
            {
                aList.Add(new TrafodionIndex(
                    aIndexedSchemaObject,
                    aReader.GetString(0).TrimEnd(),
                    aReader.GetInt64(1),
                    aReader.GetInt64(2),
                    aReader.GetInt64(3),
                    aIndexedSchemaObject.TheSecurityClass,
                    aReader.GetString(4).Trim().Replace("\0", "")));
            }

            /// <summary>
            /// Returns the OdbcDataReader that returns the Index details
            /// </summary>
            /// <param name="aConnection">an ODBC Connection</param>
            /// <param name="aIndexedSchemaObject">An indexable object.</param>
            /// <param name="objectName">The index name</param>
            /// <returns>The ODBC data reader</returns>
            override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, IndexedSchemaObject aIndexedSchemaObject, string objectName)
            {
                return Queries.ExecuteSelectIndexByName(
                        aConnection,
                        aIndexedSchemaObject.TheTrafodionSchema.TheTrafodionCatalog,
                        aIndexedSchemaObject.TheTrafodionSchema.InternalName,
                        aIndexedSchemaObject.UID,
                        aIndexedSchemaObject.TheTrafodionSchema.Version,
                        TrafodionIndex.ObjectType, TrafodionIndex.ObjectNameSpace, objectName);
            }
            /// <summary>
            /// Returns the Index model
            /// </summary>
            /// <param name="aIndexedSchemaObject">Parent object</param>
            /// <param name="aReader">The OdbcDataReader that has the resultset</param>
            /// <returns></returns>
            protected override TrafodionIndex ReadSqlObject(IndexedSchemaObject aIndexedSchemaObject, OdbcDataReader aReader)
            {
                return new TrafodionIndex(
                    aIndexedSchemaObject,
                    aReader.GetString(0).TrimEnd(),
                    aReader.GetInt64(1),
                    aReader.GetInt64(2),
                    aReader.GetInt64(3),
                    aIndexedSchemaObject.TheSecurityClass,
                    aReader.GetString(4).Trim().Replace("\0", ""));
            }
        }
    }
}
