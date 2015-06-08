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
using System.ComponentModel;
using System.Collections.Generic;
using System.Data.Odbc;
using System.Text;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Queries;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// A system message as defined on a system.
    /// </summary>
    public class SystemMessage
    {
        #region Fields

        /// <summary>
        /// The varying states of a system message and it's relationship with the system message
        /// from the system.
        /// </summary>
        public enum State
        {
            /// <summary>
            /// The system message has not yet been read.
            /// </summary>
            Uninitialized,

            /// <summary>
            /// The local copy of the system message has been updated from the system.
            /// </summary>
            Updated,

            /// <summary>
            /// The local copy of the system message is currently being updated with the system.
            /// </summary>
            Updating,

            /// <summary>
            /// An error occured during the last attempt to read the system message.
            /// </summary>
            ReadError,

            /// <summary>
            /// An error occured during the last attempt to save the system message.
            /// </summary>
            SaveError
        };

        private class WorkerResult
        {
            public string message;
            public DateTime lastupdated;
            public Exception lastException;
        }

        // Static fields.
        private static readonly string ChangedKey = "Change";

        private static ConnectionDefinition.ChangedHandler _connectionChangeHandler =
            new ConnectionDefinition.ChangedHandler(OnConnectionDefinitionChanged);

        private static Dictionary<ConnectionDefinition, SystemMessage> s_knownMessages =
            new Dictionary<ConnectionDefinition, SystemMessage>(new MyConnectionDefinitionComparer());

        // Instance fields.
        private EventHandlerList _eventHandlers = new EventHandlerList();
        private ConnectionDefinition _connectionDef;
        private DateTime _lastUpdate;
        private string _message = "";
        private bool _isDirty = false;
        private Exception _lastError = null;
        private State _state = State.Uninitialized;
        private State _oldState = State.Uninitialized;
        private BackgroundWorker _messageWorker = new BackgroundWorker();
        private const string TRACE_SUB_AREA_NAME = "System Messages";

        #endregion

        #region Properties

        /// <summary>
        /// The connection definition used to retrieve the system message.
        /// </summary>
        public ConnectionDefinition ConnectionDef
        {
            get { return _connectionDef; }
            set { _connectionDef = value; }
        }

        /// <summary>
        /// True if the system message is editable based on if the user in the
        /// connection definition is a publisher. Otherwise, false.
        /// </summary>
        public bool IsEditable
        {
            get
            {
                return IsPublisher(ConnectionDef.RoleName);
            }
        }

        /// <summary>
        /// Indicates if a local change was made to the SystemMessage.
        /// </summary>
        public bool HasUnsavedChanges
        {
            get { return _isDirty; }
        }

        /// <summary>
        /// The last time this message was updated.
        /// </summary>
        public string LastUpdateText
        {
            get
            {
                if (_lastUpdate.Ticks == DateTime.MinValue.Ticks)
                {
                    return "";
                }

                return Utilities.GetFormattedDateTime(_lastUpdate);
            }
        }

        /// <summary>
        /// The system message's text in rich-text format. Any modifications through
        /// this property is not assumed to come from the server and will cause the
        /// LastUpdate propety to indicate this fact as such.
        /// </summary>
        public string Message
        {
            get { return _message; }
            set
            {
                ChangeLocalMessage(_lastUpdate, value, false);
            }
        }

        /// <summary>
        /// State of the System Message in relation to the system with which it is associated.
        /// </summary>
        public State SynchronizationState
        {
            get { return _state; }
        }

        /// <summary>
        /// Last error encountered.
        /// </summary>
        public Exception LastError
        {
            get { return _lastError; }
        }

        #endregion Properties

        #region Events

        /// <summary>
        /// The handler of change events.
        /// </summary>
        /// <param name="sender">The control that changed.</param>
        /// <param name="oldState">The SystemMessage's state before it changed.</param>
        /// <param name="newState">The SystemMessage's state after it changed.</param>
        public delegate void ChangeHandler(SystemMessage sender, State oldState, State newState);

        /// <summary>
        /// Listeners of change events will be notified of changes made to this control;
        /// specifically to changes made to the text area.
        /// </summary>
        public event ChangeHandler Change
        {
            add { _eventHandlers.AddHandler(ChangedKey, value); }
            remove { _eventHandlers.RemoveHandler(ChangedKey, value); }
        }

        #endregion Events

        #region Static Methods

        /// <summary>
        /// Handler of changes to connection definitions.
        /// </summary>
        /// <param name="aSender">The source of the change.</param>
        /// <param name="aConnectionDefinition">The connection definition that was changed.</param>
        /// <param name="aReason">The reason for the change.</param>
        private static void OnConnectionDefinitionChanged(
            object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            SystemMessage message;
            s_knownMessages.TryGetValue(aConnectionDefinition, out message);

            // We don't know about the message yet.
            if (message == null)
            {
                return;
            }

            switch (aReason)
            {
                case ConnectionDefinition.Reason.Removed:
                    {
                        // Clean up the known list just to be consistent.
                        s_knownMessages.Remove(aConnectionDefinition);
                        return;
                    }
                default:
                    message.FireChangeEvent();
                    break;
            }
        }

        /// <summary>
        /// Returns the system message.
        /// </summary>
        /// <param name="aConnection">The ODBC connection to use.</param>
        /// <returns>The query returns a result set with the following info sorted by SEQUENCE:
        /// { SEQUENCE, LASTUPDATED, MESSAGE_TEXT }.</returns>
        static private OdbcDataReader ExecuteSelectSystemMessage(Connection aConnection)
        {
            OdbcCommand command = new OdbcCommand(string.Format(
                "SELECT SEQUENCE, LASTUPDATED, MESSAGE_TEXT " +
                "FROM {0} " +
                "ORDER BY SEQUENCE;",
                new Object[] { Manageability.SystemMessageTableName }
                ));
            command.Connection = aConnection.OpenOdbcConnection;
            return Utilities.ExecuteReader(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);
        }

        #endregion Static Methods

        /// <summary>
        /// A new SystemMessage instance.
        /// </summary>
        /// <param name="connDef">The connection definition that is used to retrieve the system message.</param>
        private SystemMessage(ConnectionDefinition connDef)
        {
            ConnectionDef = connDef;

            ConnectionDefinition.Changed += _connectionChangeHandler;

            _messageWorker.DoWork += new DoWorkEventHandler(MessageWorker_DoWork);
            _messageWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(MessageWorker_RunWorkerCompleted);
        }

        /// <summary>
        /// Retrieves a SystemMessage message object. The object may not be up-to-date
        /// with the system it is associated, so calling Refresh() may be necessary.
        /// </summary>
        /// <param name="connDef">The connection definition used to look up the SystemMessage.</param>
        /// <returns></returns>
        public static SystemMessage GetSystemMessage(ConnectionDefinition connDef)
        {
            SystemMessage message;
            s_knownMessages.TryGetValue(connDef, out message);

            if (message == null)
            {
                message = new SystemMessage(connDef);
                s_knownMessages.Add(connDef, message);
            }

            return message;
        }

        public void Reset()
        {
            _state = State.Uninitialized;
            _message = "";
            _isDirty = false;
        }

        /// <summary>
        /// Given a role name, it checks to see if the user's logged on role can modify the System Message.
        /// </summary>
        /// <param name="role">The role name to test.</param>
        /// <returns></returns>
        static public bool IsPublisher(string role)
        {
            if (role == null)
            {
                return false;
            }

            if (role.Equals("role.mgr", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
            else if (role.Equals("role.dba", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
            else if (role.Equals("super.services", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Saves the system message to the server.
        /// </summary>
        public void Save()
        {
            if (ConnectionDef.TheState != ConnectionDefinition.State.TestSucceeded)
            {
                // Do nothing.
                return;
            }

            lock (_messageWorker)
            {
                if (_state == State.Updating)
                {
                    // Work is in progress, do nothing and wait for the result.
                    return;
                }

                _oldState = _state;
                _state = State.Updating;

                FireChangeEvent();
                _messageWorker.RunWorkerAsync(true);
            }
        }

        /// <summary>
        /// Updates the message with the values from the server.
        /// </summary>
        public void Refresh()
        {
            if (ConnectionDef.TheState != ConnectionDefinition.State.TestSucceeded)
            {
                // Do nothing.
                return;
            }

            // Always check the state before kicking off the worker.
            lock (_messageWorker)
            {
                if (_state == State.Updating)
                {
                    // Work is in progress, do nothing and wait for the result.
                    return;
                }

                _oldState = _state;
                _state = State.Updating;

                FireChangeEvent();
                _messageWorker.RunWorkerAsync(false);
            }
        }

        /// <summary>
        /// Handler function, called by the background worker, to retrieve the system message.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void MessageWorker_DoWork(object sender, DoWorkEventArgs args)
        {
            // Default to no-info.
            DateTime lastupdated = new DateTime(DateTime.MinValue.Ticks);
            string message = "";
            bool doSave = (bool)args.Argument;
            SystemMessageException se = null;

            try
            {

                if (doSave)
                {
                    // Saving consists of the data doing a round-trip: from us to the system and back.
                    SaveSystemMessage();
                    GetSystemMessageFromServer(out lastupdated, out message);
                }
                else
                {
                    GetSystemMessageFromServer(out lastupdated, out message);
                }
            }
            catch (Exception e)
            {
                // Throw an exception to describe the problem.
                se = new SystemMessageException(String.Format("{0} - {1}", 
                    (_state == State.SaveError) ? Properties.Resources.UnableToSaveSystemMessage : Properties.Resources.UnableToRetrieveSystemMessage, 
                    e.Message));
            }

            WorkerResult workerResult = new WorkerResult();
            workerResult.lastupdated = lastupdated;
            workerResult.message = message;
            workerResult.lastException = se;
            args.Result = workerResult;
        }

        /// <summary>
        /// Handler for when the background worker has completed its work.
        /// </summary>
        /// <param name="sender">The background worker.</param>
        /// <param name="e"></param>
        private void MessageWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            WorkerResult result = new WorkerResult();

            // Default to no-info.
            DateTime lastupdated = new DateTime(DateTime.MinValue.Ticks);
            string message = "";

            // Extract the results.
            if (e.Result != null)
            {
                result = (WorkerResult)e.Result;
                if (result.lastException != null)
                {
                    // Set the state and notify the listeners.
                    // The result is useless.
                    if (_state != State.SaveError)
                    {
                        _state = State.ReadError;
                    }
                    _lastError = result.lastException;
                    FireChangeEvent();
                    return;
                }

                lastupdated = result.lastupdated;
                message = result.message;
            }

            // Were we cancelled?
            if (e.Cancelled)
            {
                // Revert back to the old state since the work was cancelled.
                // Don't use any of the results from the worker as its useless now.
                _state = _oldState;

                // Notify the listeners that the work is complete.
                FireChangeEvent();
                return;
            }

            // Did we have an error?
            _lastError = e.Error;
            if (_lastError != null)
            {
                // Set the state and notify the listeners.
                // The result is useless.
                _state = State.ReadError;
                FireChangeEvent();
                return;
            }

            // The work completed successfully.
            // Set the state before calling any functions. They my reject any changes until
            // the state is no longer Updating.
            _state = State.Updated;

            // Update the message. This will notify the listeners.
            ChangeLocalMessage(lastupdated, message, true);
        }

        /// <summary>
        /// Saves the local message to the system.
        /// </summary>
        private void SaveSystemMessage()
        {
            Connection connection = null;
            OdbcCommand command = new OdbcCommand("BEGIN WORK;");
            int rowsaffected;

            // Double each single quote.
            string message = Message.Replace("'", "''");

            try
            {
                connection = new Connection(ConnectionDef);
                OdbcDataReader dataReader;

                // Start our transaction.
                command.Connection = connection.OpenOdbcConnection;
                rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);

                long requiredSequenceCount = (message.Length / Manageability.MaxMessageSequenceLength) + 1;

                //// Hold the whole table.
                //command.CommandText = string.Format(
                //    "LOCK TABLE {0} IN EXCLUSIVE MODE",
                //    new Object[] { Manageability.SystemMessageTableName }
                //    );
                //rowsaffected = command.ExecuteNonQuery();

                // See how many sequences there are.
                command.CommandText = string.Format(
                    "SELECT SEQUENCE, MESSAGE_TEXT FROM {0} " +
                    "ORDER BY SEQUENCE;",
                    new Object[] { Manageability.SystemMessageTableName }
                    );
                dataReader = Utilities.ExecuteReader(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);

                long existingSequenceCount = 0;
                while (dataReader.Read())
                {
                    // Make sure that the sequences are contiguous.
                    long sequenceCol = dataReader.GetInt64(0);
                    if (sequenceCol != existingSequenceCount)
                    {
                        break;
                    }
                    else
                    {
                        existingSequenceCount = sequenceCol + 1;
                    }
                }
                dataReader.Close();

                if (requiredSequenceCount < existingSequenceCount)
                {
                    // Remove all of the sequences that we don't need.
                    command.CommandText = string.Format(
                        "DELETE FROM {0} " +
                        "WHERE SEQUENCE > {1};",
                        new Object[] { Manageability.SystemMessageTableName, requiredSequenceCount - 1 });
                    rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);

                    // Update the existing count.
                    existingSequenceCount = requiredSequenceCount;
                }

                // Put in the message as a series of sequences.
                for (int i = 0; i < requiredSequenceCount; i++)
                {
                    // Get the text that would fit into a sequence.
                    int start = i * Manageability.MaxMessageSequenceLength;
                    int end = (i < requiredSequenceCount - 1) ? Manageability.MaxMessageSequenceLength : (message.Length - (Manageability.MaxMessageSequenceLength * i));
                    string subtext = message.Substring(start, end);

                    // Insert or update based on if a sequence exists or not.
                    if (i < existingSequenceCount)
                    {
                        // The sequence exists. Update what's already there.
                        command.CommandText = string.Format(
                            "UPDATE {0} " +
                            "SET (LASTUPDATED, MESSAGE_TEXT) = ({1}, '{2}') " +
                            "WHERE SEQUENCE = {3};",
                            new Object[] { Manageability.SystemMessageTableName, "CURRENT_TIMESTAMP", subtext, i });
  
                        rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);
                    }
                    else
                    {
                        // The sequence does not exist, insert a new row.
                        command.CommandText = string.Format(
                            "INSERT INTO {0} " +
                            "(SEQUENCE, MESSAGE_TEXT) " +
                            "VALUES ({1}, '{2}');",
                            new Object[] { Manageability.SystemMessageTableName, i, subtext }
                            );
                        rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);
                    }
                }

                command.CommandText = "COMMIT WORK;";
                rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);
                _isDirty = false;
            }
            catch (OdbcException ex)
            {
                _state = State.SaveError;
                try
                {
                    command.CommandText = "ROLLBACK;";
                    rowsaffected = Utilities.ExecuteNonQuery(command, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);
                }
                catch (Exception ex1)
                {
                }
                throw ex; //throw the original exception
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }
        }

        /// <summary>
        /// Gets the system message from the 
        /// </summary>
        /// <param name="lastupdated"></param>
        /// <param name="message"></param>
        private void GetSystemMessageFromServer(out DateTime lastupdated, out string message)
        {
            Connection connection = null;
            StringBuilder messageAssembly = new StringBuilder();
            DateTime localLastUpdated = new DateTime(DateTime.MinValue.Ticks);
            
            try
            {
                connection = new Connection(ConnectionDef);
                OdbcDataReader dataReader = null;

                // Try to get the system message.
                try
                {
                    dataReader = ExecuteSelectSystemMessage(connection);
                }
                catch (OdbcException e)
                {
                    bool error1003Found = false; //schema does not exist
                    bool error4082Found = false; //table does not exist
                    foreach (OdbcError error in e.Errors)
                    {
                        if (error.NativeError == -1003)
                            error1003Found = true;
                        if (error.NativeError == -4082)
                            error4082Found = true;
                    }

                    // Does the schema (1003) or table (4082) not exist?
                    if (e.Errors.Count > 0 && (error1003Found || error4082Found))
                    {
                        // Only a publisher can create the schema and table.
                        if (IsPublisher(connection.TheConnectionDefinition.RoleName))
                        {
                            // The table doesn't exist. Try to create it.
                            // If it fails, the finally block will still get executed and
                            // we'll throw another exception.
                            Manageability.ExecuteCreateSystemMessageTable(connection, error1003Found);

                            // Try to do another select. If it fails, let the exception get
                            // thrown. We tried our best.
                            dataReader = ExecuteSelectSystemMessage(connection);
                        }
                        else
                        {
                            // A non-publisher will see nothing.
                            lastupdated = localLastUpdated;
                            message = messageAssembly.ToString();
                            return;
                        }
                    }
                    else
                    {
                        // An unexpected error. Quit.
                        if (e.Errors.Count > 0)
                        {
                            throw e;
                        }
                    }
                }

                long currentSequence = 0;

                if(dataReader != null)
                {
                    while (dataReader.Read())
                    {
                        long sequenceCol = dataReader.GetInt64(0);
                        DateTime lastUpdatedCol = dataReader.GetDateTime(1);
                        string messageCol = dataReader.GetString(2).Trim();

                        // A message is broken into contiguous sequences if it won't fit
                        // within a row. The code below assembles the message based upon
                        // a contiguous set of sequences. Once a message is assembled,
                        // no more work is required.
                        if (sequenceCol == 0)
                        {
                            // The beginning of a message existingSequenceCount.
                            localLastUpdated = lastUpdatedCol;
                            messageAssembly.Append(messageCol);
                        }
                        else if (sequenceCol == currentSequence + 1)
                        {
                            // The next existingSequenceCount.
                            messageAssembly.Append(messageCol);
                            currentSequence = sequenceCol;
                        }
                        else
                        {
                            // The sequences are out of order.
                            break;
                        }
                    }
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            // Assign the values to the out params.
            lastupdated = localLastUpdated;
            message = messageAssembly.ToString();
        }

        /// <summary>
        /// Modifies the _message field and determines how to handle the _lastUpdate field.
        /// </summary>
        /// <param name="lastUpdated">The text that indicates the last time the message was updated.</param>
        /// <param name="rtfMessage">The new message in rich-text format.</param>
        /// <param name="fromServer">True, if rtfMessage is from the server. Otherwise false.</param>
        private void ChangeLocalMessage(DateTime lastUpdated, string rtfMessage, bool fromServer)
        {
            // Do not do anything if we're in the middle of talking with the server.
            if (_state == State.Updating)
            {
                return;
            }

            // Don't allow non-publishers to modify the message if it is not from the server.
            if (_connectionDef == null || (!fromServer && !IsPublisher(_connectionDef.RoleName)))
            {
                return;
            }

            _lastUpdate = lastUpdated;
            _message = rtfMessage;
            _isDirty = !fromServer;

            FireChangeEvent();
        }

        /// <summary>
        /// Fires change events to any listeners.
        /// </summary>
        private void FireChangeEvent()
        {
            // Get the list of the right kind of handlers
            ChangeHandler changeHandlers = (ChangeHandler)_eventHandlers[ChangedKey];

            // Are any listeners?
            if (changeHandlers != null)
            {
                changeHandlers(this, _oldState, _state);
            }
        }
    }
}
