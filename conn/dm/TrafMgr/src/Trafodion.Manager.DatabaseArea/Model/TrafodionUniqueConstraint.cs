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
    /// Unqiue Constraint
    /// </summary>
    public class TrafodionUniqueConstraint : TrafodionTableConstraint
    {
        private List<string> _columns = new List<string>();

        /// <summary>
        /// Get and set the text of the Check Constraint
        /// </summary>
        public List<string> Columns
        {
            get { return _columns; }
        }        


        /// <summary>
        /// Constructs a Unqiue constraint
        /// </summary>
        /// <param name="anInternalName">Constraint name</param>
        /// <param name="anUID">UID</param>
        /// <param name="aTrafodionTable">Table for which this Unqiue constraint is defined</param>
        public TrafodionUniqueConstraint(string anInternalName, long anUID, TrafodionTable aTrafodionTable)
            : base(anInternalName, anUID, aTrafodionTable)
        {
        }

        /// <summary>
        /// Load columns of the unqiue constraints
        /// </summary>
        public void Load()
        {
            _columns = new List<string>();

            Connection theConnection = null;
            OdbcDataReader theReader = null;

            TrafodionTable aTrafodionTable = TrafodionTable as TrafodionTable;

            try
            {
                theConnection = GetConnection();
                theReader = Queries.ExecuteSelectConstraintColumnName(theConnection, aTrafodionTable.TheTrafodionCatalog.ExternalName,
                            aTrafodionTable.TheTrafodionSchema.Version, this.UID, aTrafodionTable.UID);
                while (theReader.Read())
                {
                    string columnName = theReader.GetString(0).TrimEnd();
                    _columns.Add(columnName);
                }
            }
            finally
            {
                if (theReader != null)
                {
                    theReader.Close();
                }
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// Format the unique constraint names as comma separated values
        /// </summary>
        public string FormattedColumnNames
        {
            get
            {
                System.Text.StringBuilder sb = new System.Text.StringBuilder();
                for (int i = 0; i < _columns.Count; i++ )
                {
                    sb.Append(_columns[i]);
                    if (i != _columns.Count - 1)
                    {
                        sb.Append(" , ");
                    }
                }
                return sb.ToString();
            }
        }
    }
}
