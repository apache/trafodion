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

using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// This class provides a data grid view showing all of the user's connection definitions.
    /// It can be easily subclassed to only show certain of the connection definitions that
    /// match a predicate implemented in the derived class.
    /// </summary>
    public class MySystemsDataGridView : TrafodionDataGridView
    {

        // we track changes in the connection definitions so we listen to connection definition events.
        private ConnectionDefinition.ChangedHandler _theChangedHandler;

        /// <summary>
        /// The constructor.
        /// </summary>
        public MySystemsDataGridView()
        {

            // Create the columns
            DataGridViewTextBoxColumn theStateColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theConnectionDefinitionColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theClientDataSourceColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theServerDataSourceColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theUserIDColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theUserRoleColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theHostNameColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn thePortNumberColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theDefaultSchemaColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theDriverStringColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theLiveFeedPortNumberColumn = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn theLiveFeedRetryTimerColumn = new DataGridViewTextBoxColumn();
             
            // 
            // theStateColumn
            // 
            theStateColumn.HeaderText = "State";
            theStateColumn.Name = "theStateColumn";
            // 
            // theConnectionDefinitionColumn
            // 
            theConnectionDefinitionColumn.HeaderText = "System Name";
            theConnectionDefinitionColumn.Name = "theConnectionDefinitionColumn";
            // 
            // theClientDataSourceColumn
            // 
            theClientDataSourceColumn.HeaderText = "Data Source";
            theClientDataSourceColumn.Name = "theClientDataSourceColumn";
            // 
            // theUserIDColumn
            // 
            theUserIDColumn.HeaderText = "User Name";
            theUserIDColumn.Name = "theUserIDColumn";
            // 
            // theUserSpecifiedRoleColumn
            // 
            theUserRoleColumn.HeaderText = "Primary Role Name";
            theUserRoleColumn.Name = "theUserRoleColumn";
            // 
            // theHostNameColumn
            // 
            theHostNameColumn.HeaderText = "Host";
            theHostNameColumn.Name = "theHostNameColumn";
            // 
            // thePortNumberColumn
            // 
            thePortNumberColumn.HeaderText = "ODBC Port Number";
            thePortNumberColumn.Name = "thePortNumberColumn";
            // 
            // theDefaultSchemaColumn
            // 
            theDefaultSchemaColumn.HeaderText = "Default Schema";
            theDefaultSchemaColumn.Name = "theDefaultSchemaColumn";
            // 
            // theDriverStringColumn
            // 
            theDriverStringColumn.HeaderText = "DriverString";
            theDriverStringColumn.Name = "theDriverStringColumn";
            //
            // theLiveFeedPortNumberColumn
            //
            theLiveFeedPortNumberColumn.HeaderText = "Live Feed Port Number";
            theLiveFeedPortNumberColumn.Name = "theLiveFeedPortNumberColumn";
            //
            // theLiveFeedRetryTimerColumn
            //
            theLiveFeedRetryTimerColumn.HeaderText = "Live Feed Retry Timer";
            theLiveFeedRetryTimerColumn.Name = "theLiveFeedRetryTimerColumn";

            // Add the columns
            Columns.AddRange(new DataGridViewColumn[] {
                theStateColumn,
                theConnectionDefinitionColumn,
                theClientDataSourceColumn,
                theUserIDColumn,
                //theUserRoleColumn,
                theHostNameColumn,
                thePortNumberColumn,
                theDefaultSchemaColumn,
                theDriverStringColumn
                //theLiveFeedPortNumberColumn,
                //theLiveFeedRetryTimerColumn
            });

            // Track changes in the connections
            _theChangedHandler = new ConnectionDefinition.ChangedHandler(ConnectionDefinitionChanged);
            ConnectionDefinition.Changed += _theChangedHandler;

        }

        ~MySystemsDataGridView()
        {
            if (_theChangedHandler != null)
            {
                ConnectionDefinition.Changed -= _theChangedHandler;
            }
        }

        /// <summary>
        /// Call to generate or regenerate our contents.  Lets a derived class override which
        /// conneciton definitions are included.
        /// </summary>
        public void Populate()
        {

            // Remove any current rows
            Rows.Clear();

            // Loop over all connection definitions
            foreach (ConnectionDefinition theConnectionDefinition in ConnectionDefinition.ConnectionDefinitions)
            {

                // Ask the derived class if this connection definition should be included
                if (BelongsInThisGrid(theConnectionDefinition))
                {

                    // yes .. add it to the grid
                    Rows.Add(RowData(theConnectionDefinition));

                }
            }

        }

        /// <summary>
        /// Given a connection definition, get an array of objects suitable for its row in the grid
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>The array of objects</returns>
        private object[] RowData(ConnectionDefinition aConnectionDefinition)
        {
            string userSpecifiedRole = aConnectionDefinition.UserSpecifiedRole;
            userSpecifiedRole = (userSpecifiedRole == null) ? "" : userSpecifiedRole;

            // Objects for one row
            return new object[] {
                aConnectionDefinition.StateString,
                new ConnectionDefinitionWrapper(aConnectionDefinition), // Wrapper's ToString() shows the name
                (aConnectionDefinition.ClientDataSource == null ? "" : aConnectionDefinition.ClientDataSource),
                aConnectionDefinition.UserName,
                //userSpecifiedRole,
                aConnectionDefinition.Host,
                aConnectionDefinition.Port,
                aConnectionDefinition.FullyQualifiedDefaultSchema,
                aConnectionDefinition.DriverString 
                //(aConnectionDefinition.LiveFeedPort == "-1" ? "Default Port Number" : aConnectionDefinition.LiveFeedPort),
                //aConnectionDefinition.LiveFeedRetryTimer
            };

        }

        /// <summary>
        /// Report whether or not a given connection definition belongs in this grid.  Defualt
        /// behavior is to include all, subclass can override.
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>True to include this connection definition in grid</returns>
        protected virtual bool BelongsInThisGrid(ConnectionDefinition aConnectionDefinition)
        {

            // Be default, all are shown
            return true;

        }

        /// <summary>
        /// Accessor that returns a list of all of the currently selected connection definitions
        /// </summary>
        public List<ConnectionDefinition> SelectedSystems
        {
            get
            {

                // Make an empty list
                List<ConnectionDefinition> theSelectedSystems = new List<ConnectionDefinition>();

                // Loop over all of the selected rows
                foreach (DataGridViewRow theDataGridViewRow in SelectedRows)
                {

                    // Add each  row's connection definition
                    theSelectedSystems.Add(GetConnectionDefinition(theDataGridViewRow));
                }

                // Return the list
                return theSelectedSystems;

            }
        }

        /// <summary>
        /// Returns the first currently selected connection definition.  It is assumed that the
        /// caller has verified that there is only one selection and the caller wants that one.
        /// Throws an exception if there isn't any selection at all.
        /// </summary>
        public ConnectionDefinition SelectedSystem
        {
            get
            {

                // Get the first selected row.  There must be one.
                DataGridViewRow theDataGridViewRow = SelectedRows[0];

                // Return that row's connection definition
                return GetConnectionDefinition(theDataGridViewRow);

            }
        }

        /// <summary>
        /// Given a row in the grid, return its connection definition
        /// </summary>
        /// <param name="aDataGridViewRow"></param>
        /// <returns></returns>
        public ConnectionDefinition GetConnectionDefinition(DataGridViewRow aDataGridViewRow)
        {
            return (aDataGridViewRow.Cells["theConnectionDefinitionColumn"].Value as ConnectionDefinitionWrapper).TheConnectionDefinition;
        }

        /// <summary>
        /// Called by connection definition events
        /// </summary>
        /// <param name="aSender">the sender of the event</param>
        /// <param name="aConnectionDefinition">the connection definition</param>
        /// <param name="aReason">the kind of event</param>
        void ConnectionDefinitionChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            switch (aReason)
            {
                case ConnectionDefinition.Reason.Added:
                    {
                        ConnectionDefinitionAdded(aConnectionDefinition);
                        break;
                    }
                case ConnectionDefinition.Reason.Removed:
                    {
                        ConnectionDefinitionRemoved(aConnectionDefinition);
                        break;
                    }
                default: // Edited or tested
                    {
                        ConnectionDefinitionChanged(aConnectionDefinition);
                        break;
                    }

            }
        }

        /// <summary>
        /// Track that a connection definition has been added
        /// </summary>
        /// <param name="aConnectionDefinition">a connection definition</param>
        void ConnectionDefinitionAdded(ConnectionDefinition aConnectionDefinition)
        {

            // Try to find it
            int theRowIndex = FindConnectionDefinitionRow(aConnectionDefinition);
            if (theRowIndex < 0)
            {
                // Not found, add it to the grid
                Rows.Add(RowData(aConnectionDefinition));

            }
        }

        /// <summary>
        /// Track that a  connection definition has been edited or tested
        /// </summary>
        /// <param name="aConnectionDefinition">a connection definition</param>
        void ConnectionDefinitionChanged(ConnectionDefinition aConnectionDefinition)
        {

            // Try to find it
            int theRowIndex = FindConnectionDefinitionRow(aConnectionDefinition);
            if (theRowIndex >= 0)
            {

                // Found, update its entries in the grid
                Rows[theRowIndex].SetValues(RowData(aConnectionDefinition));

            }
        }

        /// <summary>
        /// Track that a connection definition has been removed
        /// </summary>
        /// <param name="aConnectionDefinition">a connection definition</param>
        void ConnectionDefinitionRemoved(ConnectionDefinition aConnectionDefinition)
        {

            // Try to find it
            int theRowIndex = FindConnectionDefinitionRow(aConnectionDefinition);
            if (theRowIndex >= 0)
            {

                // Found, remove it from the grid
                Rows.RemoveAt(theRowIndex);

            }
        }

        /// <summary>
        /// Find the row index of a given connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">a connection definition</param>
        /// <returns>index if found else -1</returns>
        int FindConnectionDefinitionRow(ConnectionDefinition aConnectionDefinition)
        {

            // The first row would be 0
            int theRowIndex = 0;

            // Loop over the list of rows
            foreach (DataGridViewRow theDataGridViewRow in Rows)
            {

                // Get the connection definition from this row
                ConnectionDefinition theConnectionDefinition = GetConnectionDefinition(theDataGridViewRow);

                // Id it the desired one?
                if (theConnectionDefinition == aConnectionDefinition)
                {

                    // Yes, return its index
                    return theRowIndex;

                }

                // Else step to next index
                theRowIndex++;


            }

            // If we get here, we didn't find it.
            return -1;
        }

        /// <summary>
        /// Wrapper class so that we can store a connection definition directly into the grid
        /// for convenience.  Its ToName() returns the connection name so that's what shows up
        /// in the grid.
        /// </summary>
        private class ConnectionDefinitionWrapper
        {

            // Our connection definition
            private ConnectionDefinition _theConnectionDefinition;

            // Accessor for our connection definition
            internal ConnectionDefinition TheConnectionDefinition
            {
                get { return _theConnectionDefinition; }
            }

            /// <summary>
            /// Constructor
            /// </summary>
            /// <param name="aConnectionDefinition">a connection definition</param>
            internal ConnectionDefinitionWrapper(ConnectionDefinition aConnectionDefinition)
            {
                // Save the connection definition
                _theConnectionDefinition = aConnectionDefinition;
            }

            /// <summary>
            /// Returns the connection definition name
            /// </summary>
            /// <returns></returns>
            public override string ToString()
            {
                return _theConnectionDefinition.Name;
            }

        }

    }

    /// <summary>
    /// A grid showing the systems whose connection definitions have been tested and passed.
    /// </summary>
    public class MyActiveSystemsDataGridView : MySystemsDataGridView
    {

        /// <summary>
        /// Report whether or not a given connection definition belongs in this grid
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>True to include this connection definition in grid</returns>
        protected override bool BelongsInThisGrid(ConnectionDefinition aConnectionDefinition)
        {

            // We show connection definitions that have been tested successfully
            return (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded);

        }

    }

    /// <summary>
    /// A grid showing the systems whose connection definitions have NOT been tested and passed.
    /// </summary>
    public class MyOtherSystemsDataGridView : MySystemsDataGridView
    {

        /// <summary>
        /// Report whether or not a given connection definition belongs in this grid
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns>True to include this connection definition in grid</returns>
        protected override bool BelongsInThisGrid(ConnectionDefinition aConnectionDefinition)
        {

            // We show connection definitions that have been tested successfully
            return (aConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded);

        }

    }

}
