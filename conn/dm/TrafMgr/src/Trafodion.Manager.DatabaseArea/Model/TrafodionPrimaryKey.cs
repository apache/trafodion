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
    /// Primary Key Constraint
    /// </summary>
    public class TrafodionPrimaryKey : TrafodionTableConstraint
    {
        /// <summary>
        /// Constructs a Primary key constraint
        /// </summary>
        /// <param name="anInternalName">Constraint name</param>
        /// <param name="anUID">UID</param>
        /// <param name="aTrafodionTable">Table for which this primary key is defined</param>
        public TrafodionPrimaryKey(string anInternalName, long anUID, TrafodionTable aTrafodionTable)
            : base(anInternalName, anUID, aTrafodionTable)
        {
        }

        List<TrafodionPrimaryKeyColumnDef> theTrafodionPrimaryKeyColumnDefs = new List<TrafodionPrimaryKeyColumnDef>();

        /// <summary>
        /// The Primary key column definitions
        /// </summary>
        public List<TrafodionPrimaryKeyColumnDef> TheTrafodionPrimaryKeyColumnDefs
        {
            get { return theTrafodionPrimaryKeyColumnDefs; }
            set { theTrafodionPrimaryKeyColumnDefs = value; }
        }
    }

}
