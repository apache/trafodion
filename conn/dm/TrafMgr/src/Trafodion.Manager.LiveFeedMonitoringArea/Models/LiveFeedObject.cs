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
using System.Linq;
using System.Text;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Models
{
    public class LiveFeedObject
    {
        #region Fields

        ConnectionDefinition _theConnectionDefinition = null;
        Connection _theCurrentConnection = null;

        #endregion Fields

        #region Properties

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        public Connection CurrentConnection
        {
            get { return _theCurrentConnection; }
        }

        #endregion Properties

        #region Constructors

        public LiveFeedObject(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
        }

        #endregion Constructors

        /// <summary>
        /// Gets a new connection object
        /// </summary>
        /// <returns></returns>
        public bool GetConnection()
        {
            if (this._theCurrentConnection == null && this._theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _theCurrentConnection = new Connection(ConnectionDefinition);
                return true;
            }
            else
            {
                return false;
            }
        }

        public void CloseConnection()
        {
            if (this._theCurrentConnection != null)
            {
                _theCurrentConnection.Close();
                _theCurrentConnection = null;
            }
        }

    }
}


