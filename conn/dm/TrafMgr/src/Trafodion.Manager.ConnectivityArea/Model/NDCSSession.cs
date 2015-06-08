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
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Represents a NDCS server session, which is a NDCS Core server.
    /// </summary>
    public class NDCSSession : NDCSObject
    {
        #region Private member variables

        private NDCSService _theNDCSService;
        private NDCSServer _theNDCSServer;
        private int _theDialogueId;	                    //dialogue id
        private NDCSDataSource _theNDCSDataSource;
        private string _theComputerName;	            //computer name of connected client
        private string _theUserName;	                //name of connected user
        private string _theClientUserName;	                //name of connected user
        private UInt32 _theClientProcessId;             //process id of connected client
        private string _theWindowText;	                //name of connected application

        #endregion Private member variables

        #region Public properties
        /// <summary>
        /// The NDCSService associated with the session
        /// </summary>
        public NDCSService NDCSService
        {
            get { return _theNDCSService; }
            set { _theNDCSService = value; }
        }

        /// <summary>
        /// The NDCSServer associated with the session
        /// </summary>
        public NDCSServer NDCSServer
        {
            get { return _theNDCSServer; }
            set { _theNDCSServer = value; }
        }

        /// <summary>
        /// The NDCSDataSource associated with the session
        /// </summary>
        public NDCSDataSource NDCSDataSource
        {
            get { return _theNDCSDataSource; }
            set { _theNDCSDataSource = value; }
        }

        /// <summary>
        /// The computer name associated with the session
        /// </summary>
        public string ComputerName
        {
            get { return _theComputerName; }
            set { _theComputerName = value; }
        }

        /// <summary>
        /// The Client ProcessId name associated with the session
        /// </summary>
        public UInt32 ClientProcessId
        {
            get { return _theClientProcessId; }
            set { _theClientProcessId = value; }
        }

        /// <summary>
        /// The Client login name associated with the session
        /// </summary>
        public string ClientUserName
        {
            get { return _theClientUserName; }
            set { _theClientUserName = value; }
        }

        /// <summary>
        /// The User name associated with the session
        /// </summary>
        public string UserName
        {
            get { return _theUserName; }
            set { _theUserName = value; }
        }

        /// <summary>
        /// The Window Text associated with the session
        /// </summary>
        public string WindowText
        {
            get { return _theWindowText; }
            set { _theWindowText = value; }
        }

        /// <summary>
        /// The Dialogue Id associated with the session
        /// </summary>
        public int DialogueId
        {
            get { return _theDialogueId; }
            set { _theDialogueId = value; }
        }

        /// <summary>
        /// Returns the Name of the System
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Name;
        }

        /// <summary>
        /// Returns the command to replicate the current system configuration using an ALTER statement
        /// </summary>
        public override string DDLText
        {
            get
            {
                return "";
            }
        }

        /// <summary>
        /// The process name
        /// </summary>
        public override string Name
        {
            get { return NDCSServer.ProcessName; }
            set { ; }
        }

        /// <summary>
        /// ConnectionDefinition of this system
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return _theNDCSDataSource.NDCSSystem.ConnectionDefinition; }
        }

        /// <summary>
        /// Stop the session.
        /// </summary>
        public void Stop(StopMode aStopMode)
        {
            OdbcCommand anOpenCommand = null;
            try
            {
                anOpenCommand = NDCSServer.NDCSSystem.GetCommand();
                string serverName = String.Format("{0}.{1}", NDCSService.Name, NDCSServer.Port);
                int status = Queries.ExecuteStopServer(anOpenCommand, serverName, DialogueId, aStopMode);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.StoppingServerException, Name, ex.Message), ex);
            }
            finally
            {
                NDCSServer.NDCSSystem.CloseCommand(anOpenCommand);
            }
        }

        #endregion Public methods
    }
}
