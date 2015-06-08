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
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.UniversalWidget
{
    public class DefaultConnectionProvider : IConnectionProvider
    {
        // odbc session modes
        public enum SessionMode { WMSMode, CMDMode, SQLMode };

        private bool _theStickyMode = false;
        private Dictionary<string, Connection> _activeConnections = new Dictionary<string, Connection>();
        private Dictionary<string, OdbcCommand> _activeCommands = new Dictionary<string, OdbcCommand>();
        private Dictionary<string, DefaultConnectionProvider.SessionMode> _activeSessionModes = new Dictionary<string, DefaultConnectionProvider.SessionMode>();
        private string _sessionName = Connection.DefaultSessionName;

        public bool StickyMode
        {
            get { return _theStickyMode; }
            set { _theStickyMode = value; }
        }

        public void SetSessionName(string aSessionName)
        {
            _sessionName = aSessionName;
        }

        //Get a Connection object. If sticky mode is turned on, it will be obtained from the cache, else
        //a new connection will be returned.
        public Connection GetConnection(ConnectionDefinition connectionDefinition)
        {
            Connection theConnection = null;
            // check if theConnection already established
            if (_activeConnections.ContainsKey(connectionDefinition.Name))
            {
                // reuse a connection in SQL Whiteboard
                theConnection = (Connection)_activeConnections[connectionDefinition.Name];
            }
            else
            {
                theConnection = new Connection(connectionDefinition);

                theConnection.SessionName = _sessionName;

                // keep tracking all available connections established
                if (StickyMode)
                {
                    _activeConnections.Add(connectionDefinition.Name, theConnection);
                }

                //// set session mode to SQL by default
                //theCurrentSessionMode = QueryUserControl.SessionMode.SQLMode;
                //_activeSessionModes.Add(connectionDefinition.Name, theCurrentSessionMode);
            }
            return theConnection;
        }

        /// <summary>
        /// Given a Connection object, returns a ODBC Connection object
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        public OdbcConnection GetOdbcConnection(Connection aConnection)
        {
            OdbcConnection theOdbcConnection = null;
            try
            {
                theOdbcConnection = aConnection.OpenOdbcConnection;
            }
            catch (Exception oe)
            {
                //If open connection fails, remove connection from dictionary
                if (aConnection != null)
                {
                    aConnection.Close();
                    _activeConnections.Remove(aConnection.TheConnectionDefinition.Name);
                    _activeSessionModes.Remove(aConnection.TheConnectionDefinition.Name);
                    _activeCommands.Remove(aConnection.TheConnectionDefinition.Name);
                }

                //Rethrow exception so background worker can handle it.
                throw new Exception("Error opening connection: " + oe.Message + Environment.NewLine +
                    "Check the connection and schema name settings.");
            }
            return theOdbcConnection;
        }

        /// <summary>
        /// Returns a OdbcCommand for a connection object.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aOdbcConnection">It can be null</param>
        /// <returns></returns>
        public OdbcCommand GetCommand(Connection aConnection, OdbcConnection aOdbcConnection)
        {
            OdbcCommand command = null;
            if (aOdbcConnection == null)
            {
                aOdbcConnection = GetOdbcConnection(aConnection);
            }
            // check if a command already established
            if (_activeCommands.ContainsKey(aConnection.TheConnectionDefinition.Name))
            {
                // reuse a command in SQL Whiteboard
                command = (OdbcCommand)_activeCommands[aConnection.TheConnectionDefinition.Name];

                command.Connection = aOdbcConnection;
                // but, remember to clean it up first
                command.Parameters.Clear();
            }
            else
            {
                command = aOdbcConnection.CreateCommand();
                command.CommandType = CommandType.Text;

                // keep tracking all available connections established
                if (StickyMode)
                {
                    _activeCommands.Add(aConnection.TheConnectionDefinition.Name, command);
                }
            }
            return command;
        }

        public void ReleaseConnection(Connection aConnection)
        {
            if (StickyMode)
            {
                //do nothing
            }
            else
            {
                if (aConnection != null)
                {
                    aConnection.Close();
                    _activeConnections.Remove(aConnection.TheConnectionDefinition.Name);
                    _activeSessionModes.Remove(aConnection.TheConnectionDefinition.Name);
                    _activeCommands.Remove(aConnection.TheConnectionDefinition.Name);
                }
            }
        }

        public void ClearConnection(Connection aConnection)
        {
            if (aConnection != null)
            {
                aConnection.Close();
                _activeConnections.Remove(aConnection.TheConnectionDefinition.Name);
                _activeSessionModes.Remove(aConnection.TheConnectionDefinition.Name);
                _activeCommands.Remove(aConnection.TheConnectionDefinition.Name);
            }
        }

        public DefaultConnectionProvider.SessionMode GetSessionMode(ConnectionDefinition connectionDefinition)
        {
            if (!_activeSessionModes.ContainsKey(connectionDefinition.Name))
            {
                _activeSessionModes.Add(connectionDefinition.Name, DefaultConnectionProvider.SessionMode.SQLMode);
            }
            return _activeSessionModes[connectionDefinition.Name];
        }

        public void SetSessionMode(ConnectionDefinition connectionDefinition, DefaultConnectionProvider.SessionMode aCurrentSessionMode)
        {
            if (_activeSessionModes.ContainsKey(connectionDefinition.Name))
            {
                _activeSessionModes.Remove(connectionDefinition.Name);
            }
            _activeSessionModes.Add(connectionDefinition.Name, aCurrentSessionMode);
        }

        public void Cleanup()
        {
            foreach (Connection connection in _activeConnections.Values)
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }
            _activeConnections.Clear();
            _activeSessionModes.Clear();

        }
    }
}
