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

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// This class represents a NDCS server
    /// </summary>
    public class NDCSServer
    {
        #region Private member variables

        private short _theNodeId;
        private long _theProcessId;
        private ServerStateEnum _theServerState;
        private string _theProcessName;
        private int _thePort;
        private long _theLastUpdatedTime;
        private short _theComponentIDVersion;
        private short _theMajorVersion;
        private short _theMinorVersion;
        private int _theBuildID;
        private ServerTypeEnum _theServerType;
        private NDCSSystem _theNDCSSystem;

        #endregion Private member variables

        #region Public types and properties

        /// <summary>
        /// ENUM ServerStateEnum: defines the state of a NDCS Server.
        /// The most common states of association server are SRVR_AVAILABLE (service is running) 
        /// and SRVR_STOPPED (service is stopped).
        /// </summary>
        public enum ServerStateEnum : int
        {
            SRVR_UNINITIALIZED = 0,
            SRVR_STARTING,
            SRVR_STARTED,
            SRVR_AVAILABLE,
            SRVR_CONNECTING,
            SRVR_CONNECTED,
            SRVR_DISCONNECTING,
            SRVR_DISCONNECTED,
            SRVR_STOPPING,
            SRVR_STOPPED,
            SRVR_ABENDED,
            SRVR_CONNECT_REJECTED,
            SRVR_CONNECT_FAILED,
            SRVR_CLIENT_DISAPPEARED,
            SRVR_ABENDED_WHEN_CONNECTED,
            SRVR_STATE_FAULT,
            SRVR_STOP_WHEN_DISCONNECTED
        }

        /// <summary>
        /// The display names of the server states
        /// </summary>
        static public string[] ServerStateNames = new string[] 
        {
            "Uninitialized", "Starting", "Started", "Available", "Connecting", "Connected", "Disconnecting", 
            "Disconnected", "Stopping", "Stopped", "Abended", "Connect Rejected", "Connect Failed", 
            "Client Disappeared", "Abended When Connected", "State Fault", "Stop When Disconnected" 
        };

        /// <summary>
        /// ENUM ServerTypeEnum defines the NDCS Server types.
        /// </summary>
        public enum ServerTypeEnum : int 
        {
            SRVR_UNKNOWN = 0,
            AS_SRVR,
            CORE_SRVR,
            CFG_SRVR
        }

        /// <summary>
        /// The process id of the server
        /// </summary>
        public long ProcessId
        {
            get { return _theProcessId; }
            set { _theProcessId = value; }
        }

        /// <summary>
        /// The state of the server
        /// </summary>
        public ServerStateEnum ServerState
        {
            get { return _theServerState; }
            set { _theServerState = value; }
        }

        /// <summary>
        /// The process name of the server 
        /// </summary>
        public string ProcessName
        {
            get { return _theProcessName; }
            set { _theProcessName = value; }
        }

        /// <summary>
        /// The server port number
        /// </summary>
        public int Port
        {
            get { return _thePort; }
            set { _thePort = value; }
        }

        /// <summary>
        /// The time the server was last updated
        /// </summary>
        public long LastUpdatedTime
        {
            get { return _theLastUpdatedTime; }
            set { _theLastUpdatedTime = value; }
        }

        /// <summary>
        /// The NDCS server version
        /// </summary>
        public short ComponentIDVersion
        {
            get { return _theComponentIDVersion; }
            set { _theComponentIDVersion = value; }
        }

        /// <summary>
        /// The NDCS server version: Major
        /// </summary>
        public short MajorVersion
        {
            get { return _theMajorVersion; }
            set { _theMajorVersion = value; }
        }

        /// <summary>
        /// The NDCS server version: Minor
        /// </summary>
        public short MinorVersion
        {
            get { return _theMinorVersion; }
            set { _theMinorVersion = value; }
        }

        /// <summary>
        /// The NDCS server version: Build id
        /// </summary>
        public int BuildID
        {
            get { return _theBuildID; }
            set { _theBuildID = value; }
        }

        /// <summary>
        /// The node on which the server is running
        /// </summary>
        public short NodeId
        {
            get { return _theNodeId; }
            set { _theNodeId = value; }
        }

        /// <summary>
        /// The type of the server. Association/Configuration
        /// </summary>
        public ServerTypeEnum ServerType
        {
            get { return _theServerType; }
            set { _theServerType = value; }
        }

        /// <summary>
        /// The System where the server is running
        /// </summary>
        public NDCSSystem NDCSSystem
        {
            get { return _theNDCSSystem; }
            set { _theNDCSSystem = value; }
        }

        #endregion Public types and properties

        #region Constructor
        public NDCSServer(NDCSSystem aSystem, ServerTypeEnum aServerType)
        {
            _theNDCSSystem = aSystem;
            ServerType = aServerType;

        }
        #endregion constructor
    }
}
