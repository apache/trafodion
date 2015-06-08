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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;

namespace Trafodion.Manager.Framework.Connections
{
    public class ConnectionRegistry
    {
        private object locker = new object();
        Dictionary<string, Stack<OdbcConnection>> connectionTable = new Dictionary<string, Stack<OdbcConnection>>();
        Dictionary<string, Stack<OdbcConnection>> adminConnectionTable = new Dictionary<string, Stack<OdbcConnection>>();
        private static ConnectionRegistry _Instance = new ConnectionRegistry();
        int _maxConnectionStackDepth = 5;
        int _maxAdminConnectionStackDepth = 1;
        private ConnectionDefinition.ChangedHandler _connectionChangeHandler;

        private ConnectionRegistry()
        {
            _connectionChangeHandler = new ConnectionDefinition.ChangedHandler(OnConnectionDefinitionChanged);
            ConnectionDefinition.Changed += _connectionChangeHandler;
        }

        ~ConnectionRegistry()
        {
            ConnectionDefinition.Changed -= _connectionChangeHandler;
        }

        /// <summary>
        /// Get handle to the Singleton
        /// </summary>
        /// <returns></returns>
        public static ConnectionRegistry Instance
        {
            get
            {
                return _Instance;
            }
        }

        /// <summary>
        /// Returns a ODBC Connection from the stack or creates a new one and returns it
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="serverDsName"></param>
        /// <returns></returns>
        public OdbcConnection GetODBCConnection(Connection aConnection, string serverDsName)
        {
            lock (locker)
            {
                Dictionary<string, Stack<OdbcConnection>> table = (aConnection.IsAdminConnection) ? adminConnectionTable : connectionTable;
                if (table.ContainsKey(aConnection.TheConnectionDefinition.Name))
                {
                    Stack<OdbcConnection> connections = table[aConnection.TheConnectionDefinition.Name] as Stack<OdbcConnection>;
                    if ((connections != null) && (connections.Count > 0))
                    {
                        OdbcConnection con = GetValidConnection(connections);
                        //If we get an invalid connection, we will go back to the stack and get the next connection
                        while ((con == null) && (connections.Count > 0))
                        {
                            con = GetValidConnection(connections);
                        }

                        //If we got a valid connection, return it
                        if (con != null)
                        {
                            return con;
                        }
                    }
                }
            }
            //now if there is none in the stack, lets create one
            return aConnection.GetODBCConnection(serverDsName);
        }

        private OdbcConnection GetValidConnection(Stack<OdbcConnection> connections)
        {
            OdbcConnection con = connections.Pop();
            if (IsConnectionValid(con))
            {
                return con;
            }
            return null;
        }

        /// <summary>
        /// Check if an Admin odbc connection exists
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        public bool AdminConnectionExists(ConnectionDefinition aConnectionDefinition)
        {
            lock (locker)
            {
                Dictionary<string, Stack<OdbcConnection>> table = adminConnectionTable;
                foreach (KeyValuePair<string, Stack<OdbcConnection>> de in table)
                {
                    if (de.Key.Equals(aConnectionDefinition.Name))
                    {
                        Stack<OdbcConnection> connections = de.Value;
                        foreach (OdbcConnection con in connections)
                        {
                            if (IsConnectionValid(con))
                                return true;
                        }
                    }
                }
                return false;
            }
        }

        /// <summary>
        /// Check if non admin odbc connection exists
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        public bool NonAdminConnectionExists(ConnectionDefinition aConnectionDefinition)
        {
            lock (locker)
            {
                Dictionary<string, Stack<OdbcConnection>> table = connectionTable;
                foreach (KeyValuePair<string, Stack<OdbcConnection>> de in table)
                {
                    if (de.Key.Equals(aConnectionDefinition.Name))
                    {
                        Stack<OdbcConnection> connections = de.Value;
                        foreach (OdbcConnection con in connections)
                        {
                            if (IsConnectionValid(con))
                                return true;
                        }
                    }
                }
                return false;
            }
        }

        /// <summary>
        /// Check if odbc connection exists for the given connection definition
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        public bool ConnectionExists(ConnectionDefinition aConnectionDefinition)
        {
            bool connectionExists = NonAdminConnectionExists(aConnectionDefinition);

            if (connectionExists)
            {
                return true;
            }
            else
            {
                return AdminConnectionExists(aConnectionDefinition);
            }
        }

        public void CloseODBCConnection(Connection aConnection, OdbcConnection aODBCConnection)
        {
            lock (locker)
            {
                bool canPush = true;
                Dictionary<string, Stack<OdbcConnection>> table = (aConnection.IsAdminConnection) ? adminConnectionTable : connectionTable;
                Stack<OdbcConnection> connections = null;
                int stackSize = (aConnection.IsAdminConnection) ? _maxAdminConnectionStackDepth : _maxConnectionStackDepth;

                if ((aConnection.IsConnectionOpen) && (aConnection.TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded))
                {
                    if (table.ContainsKey(aConnection.TheConnectionDefinition.Name))
                    {
                       connections = table[aConnection.TheConnectionDefinition.Name] as Stack<OdbcConnection>;
                        if (connections.Count >= stackSize)
                        {
                            canPush = false;
                        }
                    }
                    else
                    {
                        connections = new Stack<OdbcConnection>(stackSize);
                        table.Add(aConnection.TheConnectionDefinition.Name, connections);
                    }
                }
                else
                {
                    //Console.WriteLine(String.Format("ODBC connection is in a closed state. Is admin =  {0}", aConnection.IsAdminConnection));
                    canPush = false;
                }

                //If the stack has capacity, add it to the stack
                if (canPush)
                {
                    connections.Push(aODBCConnection);
                }
                else
                {
                    //Console.WriteLine(String.Format("Stack is full, hence destroying {0} . Is admin =  {1}" , (connections != null) ? connections.Count : 0, aConnection.IsAdminConnection));
                    closeAndReleaseConnection(aODBCConnection);
                }

            }
        }

        /// <summary>
        /// Checks the status of the connection object
        /// </summary>
        /// <param name="aOdbcConnection"></param>
        /// <returns></returns>
        private bool IsConnectionValid(OdbcConnection aOdbcConnection)
        {
            bool valid = false;
            if (aOdbcConnection != null)
            {
                //If the connection is gone just do the cleanup
                if (aOdbcConnection.State == ConnectionState.Open) 
                {
                    valid = callTestSQL(aOdbcConnection);
                }
                else if ((aOdbcConnection.State == ConnectionState.Broken) 
                    || (aOdbcConnection.State == ConnectionState.Closed))
                {
                    valid = false;
                    // Releases all resources used by this ODBC connection.. 
                    aOdbcConnection.Dispose();
                }
                else if ((aOdbcConnection.State == ConnectionState.Connecting) 
                    || (aOdbcConnection.State == ConnectionState.Executing) 
                    || (aOdbcConnection.State == ConnectionState.Fetching))
                {
                    valid = false;
                    //This should not be the case. What do we do?
                    //I guess just close and release the resources
                    closeAndReleaseConnection(aOdbcConnection);
                }
            }
            else
            {
                valid = false;
            }
            return valid;
        }

        private bool callTestSQL(OdbcConnection aOdbcConnection)
        {
            bool ret = true;
            OdbcCommand command = null;
            try
            {
                command = aOdbcConnection.CreateCommand();

                //command.CommandText = "WMSOPEN";
                //command.ExecuteNonQuery();

                // This is just throwing a test of the connection, there is no point of recording this at all.
                // So, we just execute this out of the command rather than going through Utilities.ExecuteNonQuery.
                command.CommandText = "WMSCLOSE";
                Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, "Test Connection Validity", true);
            }
            catch (Exception ex)
            {
                //Console.WriteLine("Connection is invalid, hence destroying.");
                ret = false;
                closeAndReleaseConnection(aOdbcConnection);
            }
            finally
            {
                if (command != null)
                {
                    command.Dispose();
                }
            }
            return ret;
        }

        private void closeAndReleaseConnection(OdbcConnection aOdbcConnection)
        {
            //close the connecction
            try
            {
                // We do; close it
                aOdbcConnection.Close();

                // Releases all resources used by this ODBC connection.. 
                aOdbcConnection.Dispose();

                // Notify the ODBC Driver Manager that environment handle can be released when the last underlying connection is released.
                OdbcConnection.ReleaseObjectPool();
            }
            catch (Exception ex)
            {
                //do nothiong
            }
        }

        /// <summary>
        /// Handler of changes to connection definitions.
        /// </summary>
        /// <param name="aSender">The source of the change.</param>
        /// <param name="aConnectionDefinition">The connection definition that was changed.</param>
        /// <param name="aReason">The reason for the change.</param>
        private void OnConnectionDefinitionChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason != ConnectionDefinition.Reason.Tested)
            {
                lock (locker)
                {
                    if (aConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                    {
                        //Clean the admin connections
                        RemoveConnectionDefinition(adminConnectionTable, aConnectionDefinition);

                        //Clean the other connections
                        RemoveConnectionDefinition(connectionTable, aConnectionDefinition);
                    }
                }
            }            
        }

        /// <summary>
        /// Closes all connections for the connection definition and 
        /// removes the connection definition from the dictionary
        /// </summary>
        /// <param name="table"></param>
        /// <param name="aConnectionDefinition"></param>
        void RemoveConnectionDefinition(Dictionary<string, Stack<OdbcConnection>> table, 
                                                ConnectionDefinition aConnectionDefinition)
        {
            if (table.ContainsKey(aConnectionDefinition.Name))
            {
                Stack<OdbcConnection> connections = table[aConnectionDefinition.Name];
                CloseAndReleaseAllConnections(connections);
                table.Remove(aConnectionDefinition.Name);
            }
        }

        /// <summary>
        /// This method closes all connection in this stack
        /// </summary>
        /// <param name="connections"></param>
        private void CloseAndReleaseAllConnections(Stack<OdbcConnection> connections)
        {
            if ((connections != null) && (connections.Count > 0))
            {
                OdbcConnection con = connections.Pop();
                while (con != null)
                {
                    closeAndReleaseConnection(con);
                    con = (connections.Count > 0) ? connections.Pop() : null;
                }
            }
        }

        public void CloseAndReleaseAllConnections()
        {
            foreach (ConnectionDefinition connectionDefinition in ConnectionDefinition.ConnectionDefinitions)
            {
                if (connectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                    continue;

                //Clean the admin connections
                RemoveConnectionDefinition(adminConnectionTable, connectionDefinition);

                //Clean the other connections
                RemoveConnectionDefinition(connectionTable, connectionDefinition);
            }
        }
    }
}
