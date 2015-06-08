// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.Text;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.UniversalWidget
{
    public interface IConnectionProvider
    {
        bool StickyMode
        {
            get;
            set;
        }
        Connection GetConnection(ConnectionDefinition connectionDefinition);
        OdbcConnection GetOdbcConnection(Connection aConnection);
        OdbcCommand GetCommand(Connection aConnection, OdbcConnection aOdbcConnection);
        DefaultConnectionProvider.SessionMode GetSessionMode(ConnectionDefinition connectionDefinition);
        void SetSessionMode(ConnectionDefinition connectionDefinition, DefaultConnectionProvider.SessionMode aSessionMode);

        void SetSessionName(string aSessionName);
        void ReleaseConnection(Connection aConnection);
        void ClearConnection(Connection aConnection);
        void Cleanup();
    }
}
