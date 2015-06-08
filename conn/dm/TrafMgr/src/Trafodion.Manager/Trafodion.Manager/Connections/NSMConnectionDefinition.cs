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
using System.Collections;
using System.Text;
using System.Windows.Forms;
using System.Data.Odbc;
using System.ComponentModel;
using System.Runtime.Serialization;
using System.IO;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections
{

    /// <summary>
    /// The definition of a connection.  The user sees these as "Systems" but there's not a one-to-one
    /// correspondence since there might be several connection definitions that refer to a given system.
    /// There might be one connection defintion using the services user and another using a regular
    /// user for instance.  A connection definition associates a user-chosen
    /// name with the various fields such as username, password, ip address, port, that are are 
    /// needed to make an ODBC connection string.
    /// </summary>
    [Serializable]
    public class NSMConnectionDefinition : ConnectionDefinition
    {
        public new static int TheMinPortNumber
        {
            get { return 1; }
        }

        public new static int TheDefaultPortNumber
        {
            get { return 18650; }
        }

        public new static int TheMaxPortNumber
        {
            get { return 65535; }
        }

        public new NSMConnection TheSystemMonitorConnection
        {
            get {
                return _TheSystemMonitorConnection;
            }
        }


        private new int _NSMPort = 4746;

        /// <summary>
        /// The host name.
        /// </summary>
        public new int NSMPort
        {
            get { return _NSMPort; }
            set
            {

                // Eliminate no-change events
                if (_NSMPort.Equals(value))
                {
                    return;
                }

                //theState = State.NotTested;
                _NSMPort = value;
                //FireChanged(Reason.Host);
            }
        }

        private NSMConnection _TheSystemMonitorConnection;

        /// <summary>
        /// Creates a new uninitialized connection definition
        /// </summary>
        public NSMConnectionDefinition() : base()
        {

            // Always start with the default port number
            //thePort = TheDefaultPortNumber.ToString();

            // And the default driver
            //theDriverString = TheDefaultHpOdbcDriverString;


        }

        /// <summary>
        /// Creates a new instance connection definition that is a copy of an existing one
        /// </summary>
        public NSMConnectionDefinition(ConnectionDefinition aConnectionDefinition)
            : base()
        {

            SuppressEvents();
            Set(aConnectionDefinition);
            _TheSystemMonitorConnection = new NSMConnection(this);
            AllowEvents();
            //Set(aConnectionDefinition);
        }
    }
}
