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

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Abstract class for Indexed Schema Objects
    /// </summary>
    abstract public class IndexedSchemaObject : PartitionedSchemaObject
    {
        private IndexList _sqlMxIndexes = null;

        /// <summary>
        /// Constructor for IndexedSchemaObject
        /// </summary>
        /// <param name="aTrafodionSchema">the Schema</param>
        /// <param name="anInternalName">the Internal Name</param>
        /// <param name="aUID">the UID</param>
        /// <param name="aCreateTime">the Create Time</param>
        /// <param name="aRedefTime">the Redefinition Time</param>
        /// <param name="aSecurityClass">the Security Class</param>
        /// <param name="anOwner">the owner of the indexed schema object</param>
        public IndexedSchemaObject(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
        }

        /// <summary>
        /// Clears the list of indexes.
        /// </summary>
        public void RefreshIndexes()
        {
            if (_sqlMxIndexes != null)
            {
                _sqlMxIndexes.Refresh();
            }
        }

        /// <summary>
        /// Loads a specific index given is name
        /// </summary>
        /// <param name="anIndexName"></param>
        /// <returns></returns>
        public TrafodionIndex LoadIndexByName(string anIndexName)
        {
            return new IndexList.TrafodionIndexesLoader().LoadObjectByName(this, anIndexName);
        }

        /// <summary>
        /// A list of indexes.
        /// </summary>
        public List<TrafodionIndex> TrafodionIndexes
        {
            get
            {
                if (_sqlMxIndexes == null)
                {
                    _sqlMxIndexes = new IndexList(this);
                }
                return _sqlMxIndexes;
            }
        }

        /// <summary>
        /// Resets this object
        /// </summary>
        override public void Refresh()
        {
            base.Refresh();
            RefreshIndexes();
        }
    }
}
