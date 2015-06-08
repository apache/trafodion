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
using System.Text;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// TrafodionIndex column 
    /// </summary>
    public class TrafodionIndexColumn : TrafodionObject
    {

        // Member variables
        private TrafodionIndex _sqlMxIndex;
        private bool _theSystemAddedColumn;
        private bool _isAscending;

        /// <summary>
        /// Creates a new instance of TrafodionIndexColumn
        /// </summary>
        /// <param name="aTrafodionIndex"></param>
        /// <param name="anExternalName"></param>
        /// <param name="anOrdering"></param>
        /// <param name="aSystemAddedColumn"></param>       
        public TrafodionIndexColumn(TrafodionIndex aTrafodionIndex, string anExternalName, string anOrdering, string aSystemAddedColumn)
            :base(anExternalName, aTrafodionIndex.UID)
        {
            ExternalName = anExternalName;
            _sqlMxIndex = aTrafodionIndex;
            _isAscending = anOrdering.Trim().Equals("A");
            _theSystemAddedColumn = aSystemAddedColumn.Trim().Equals("Y");
        }

        /// <summary>
        /// Returns the TrafodionIndex object this TrafodionIndexColumn references
        /// </summary>
        public TrafodionIndex TheTrafodionIndex
        {
            get { return _sqlMxIndex; }            
        }

        /// <summary>
        /// Returns the ordering of the TrafodionIndexColumn
        /// </summary>
        public bool IsAscending
        {
            get { return _isAscending; }
        }

        /// <summary>
        /// the system added column
        /// </summary>
        public bool TheSystemAddedColumn
        {
            get { return _theSystemAddedColumn; }
        }

        /// <summary>
        /// Returns the connection from the index
        /// </summary>
        /// <returns></returns>
        override public Connection GetConnection()
        {
            return TheTrafodionIndex.GetConnection();
        }

        /// <summary>
        /// Returns the connection definition from the index
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return TheTrafodionIndex.ConnectionDefinition; }
        }



    }
}
