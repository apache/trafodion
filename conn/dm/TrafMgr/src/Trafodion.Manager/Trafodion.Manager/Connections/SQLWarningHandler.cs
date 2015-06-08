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

using System.Data;
using System.Data.Odbc;

namespace Trafodion.Manager.Framework.Connections
{
    public class SQLWarningHandler
    {
        private OdbcInfoMessageEventHandler _odbcInfoMessageHandler = null;
        private DataTable _warningsDataTable;

        public DataTable WarningsDataTable
        {
            get { return _warningsDataTable; }
        }

        public SQLWarningHandler(OdbcConnection aOdbcConnection)
        {
            aOdbcConnection.InfoMessage += OdbcConnection_InfoMessageHandler;
            _warningsDataTable = new DataTable();
        }

        void OdbcConnection_InfoMessageHandler(object sender, OdbcInfoMessageEventArgs e)
        {
            _warningsDataTable = new DataTable();
            _warningsDataTable.Columns.Add("Warning Text");

            foreach (OdbcError oe in e.Errors)
            {
                _warningsDataTable.Rows.Add(new string[] { oe.Message });
            }
        }

        public void UnRegister(OdbcConnection aOdbcConnection)
        {
            aOdbcConnection.InfoMessage -= OdbcConnection_InfoMessageHandler; 
        }
    }
}
