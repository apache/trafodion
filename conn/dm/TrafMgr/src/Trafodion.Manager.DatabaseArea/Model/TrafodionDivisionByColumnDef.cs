//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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

using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// A column definition for the DivisionBy
    /// </summary>
    public class TrafodionDivisionByColumnDef : TrafodionObject
    {

        // Member variables
        private int _sequenceNumber;
        private TrafodionSchemaObject _sqlMxSchemaObject;
        private int _columnNumber;
        private string _expression;
        private bool _isAscending;
        private bool _theSystemAddedColumn;

        /// <summary>
        /// Creates a new instance of a TrafodionDivisionByColumnDef
        /// </summary>
        /// <param name="aTrafodionSchemaObject">the TrafodionSchemaObject</param>
        /// <param name="anExternalName">the External Name</param>
        /// <param name="aPositionInRow">the row position</param>
        /// <param name="anOrdering">the Ordering type</param>
        /// <param name="aSystemAddedColumn">whether the column was system added</param>       
        public TrafodionDivisionByColumnDef(TrafodionSchemaObject aTrafodionSchemaObject, int sequenceNumber, int columnNumber, string anExternalName, string anOrdering, 
            string anExpression, string aSystemAddedColumn)
            : base(anExternalName, aTrafodionSchemaObject.UID)
        {
            ExternalName = anExternalName;
            _sqlMxSchemaObject = aTrafodionSchemaObject;
            _sequenceNumber = sequenceNumber;
            _columnNumber = columnNumber;
            _expression = anExpression;
            _isAscending = anOrdering.Trim().Equals("A");
            _theSystemAddedColumn = aSystemAddedColumn.Trim().Equals("Y");
        }

        /// <summary>
        /// Returns the TrafodionTable
        /// </summary>
        public TrafodionSchemaObject TheTrafodionSchemaObject
        {
            get { return _sqlMxSchemaObject; }            
        }

        /// <summary>
        /// The position or sequence number of the division by column
        /// </summary>
        public int SequenceNumber
        {
            get { return _sequenceNumber; }
        }

        /// <summary>
        /// Returns the ordering of the TrafodionDivisionByColumnDef
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
        /// Returns the Expression of the TrafodionDivisionByColumnDef
        /// </summary>
        public string Expression
        {
            get { return _expression; }
        }

        /// <summary>
        /// Column number of the division by column
        /// </summary>
        public int ColumnNumber
        {
            get { return _columnNumber; }
        }

        /// <summary>
        /// Returns the connection from the DivisionBy Def
        /// </summary>
        /// <returns></returns>
        override public Connection GetConnection()
        {
            return TheTrafodionSchemaObject.GetConnection();
        }

        /// <summary>
        /// Returns the connection definition from the Table
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return TheTrafodionSchemaObject.ConnectionDefinition; }
        }


    }
}
