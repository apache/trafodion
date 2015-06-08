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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// This user control shows a grid with all of the user's systems (connection definitions) and
    /// a set of buttons that can be user to test, edit, add new, add like, remove, and clear passwords
    /// for selected systens.
    /// <para/>
    /// This user control can be easily subclassed to provide a user control that only shows a subset
    /// of the user's systems based on a predicate implemented in the derived class.
    /// </summary>
    public partial class MySystemsUserControl : UserControl
    {

        // The actual dat gid view of systems
        MySystemsDataGridView _theMySystemsDataGridView;

        /// <summary>
        /// Default constructor that uses a data grid view of ALL systems
        /// </summary>
        public MySystemsUserControl()
        {

            // Create a standard systems data grid view
            _theMySystemsDataGridView = new MySystemsDataGridView();

            // And call the common constructor code
            CommonConstruction();

        }

        public MySystemsUserControl(bool doPopulate)
        {
            // Create a standard systems data grid view
            _theMySystemsDataGridView = new MySystemsDataGridView();

            // And call the common constructor code
            CommonConstruction();

            if (doPopulate)
            {
                Populate();
            }
        }
        /// <summary>
        /// Constructor that lets the caller provide the data grid view.  This constructor is 
        /// used by derived classes to provide data grids with custom predicates.
        /// </summary>
        /// <param name="aMySystemsDataGridView">a class derived from MySystemsDataGridView</param>
        public MySystemsUserControl(MySystemsDataGridView aMySystemsDataGridView)
        {

            // Accept the caller-supplied systems  data grid view
            _theMySystemsDataGridView = aMySystemsDataGridView;

            // And call the common constructor code
            CommonConstruction();

        }

        /// <summary>
        /// The common construction code
        /// </summary>
        private void CommonConstruction()
        {

            // Call the designer-generated code
            InitializeComponent();

            // We need to know when the selection in the grid changes
            _theMySystemsDataGridView.SelectionChanged += new EventHandler(TheMySystemsDataGridViewSelectionChanged);

            // We need to know when the user double clicks
            _theMySystemsDataGridView.CellMouseDoubleClick += new DataGridViewCellMouseEventHandler(MySystemsDataGridViewCellMouseDoubleClick);

            // This will case the systems data grid to fill the space above the buttons
            _theMySystemsDataGridView.Dock = DockStyle.Fill;
            gridPanel.Controls.Add(_theMySystemsDataGridView);

            UpdateControls();

        }

        /// <summary>
        /// Called when the user double clicks a cell in the grid.  It is handled as though the
        /// user selected the row and clicked the Edit button.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void MySystemsDataGridViewCellMouseDoubleClick(object sender, DataGridViewCellMouseEventArgs e)
        {

            // Let the user edit the row's connection definition
            if (e.RowIndex >= 0)
            {
                DoEdit(_theMySystemsDataGridView.GetConnectionDefinition(_theMySystemsDataGridView.Rows[e.RowIndex]));
            }

        }

        /// <summary>
        /// Called when the user changes the selection in the grid
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TheMySystemsDataGridViewSelectionChanged(object sender, EventArgs e)
        {

            // Make the controls consistent with the selection
            UpdateControls();

        }

        /// <summary>
        /// Makes the controls consistent with the selection
        /// </summary>
        private void UpdateControls()
        {

            // True if any rows are selected
            bool anyRowsSelected = (_theMySystemsDataGridView.SelectedRows.Count > 0);

            // True if exactly one row is selected
            bool oneRowSelected = (_theMySystemsDataGridView.SelectedRows.Count == 1);

            //True if there is a connected row has been selected.
            bool anyConnectedRowSelected = false;

            foreach (ConnectionDefinition cd in _theMySystemsDataGridView.SelectedSystems)
            {
                if (cd.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    anyConnectedRowSelected = true;
                    break;
                }
            }

            // Can test 1 or more systems at a time
            _theTestButton.Enabled = anyRowsSelected && anyConnectedRowSelected;

            // Can edit only 1 system at a time
            _theEditButton.Enabled = oneRowSelected;

            // Can always add a system
            _theAddButton.Enabled = true;

            // Can add a system like another 1 system
            _theAddLikeButton.Enabled = oneRowSelected;

            // Can remove 1 or more systems at a time
            _theRemoveButton.Enabled = anyRowsSelected;

            // Can clear the passwords for 1 or more systems at a time
            _theDisconnectButton.Enabled = anyRowsSelected && anyConnectedRowSelected;

        }

        /// <summary>
        /// Clears and re-populates the data grid
        /// </summary>
        public void Populate()
        {

            // The grid does all of the work
            _theMySystemsDataGridView.Populate();

        }

        /// <summary>
        /// Remove a list of systems after asking the user to confirm.
        /// </summary>
        /// <param name="aConnectionDefinitionList"></param>
        private void DoRemoves(List<ConnectionDefinition> aConnectionDefinitionList)
        {

            // Check if some cloned windows are opened--display appropriate messsages e.g. 
            // Do you really want to remove the selected system(s)?
            // Close all clone-windows and, then only Remove the systems  
            if (WindowsManager.ClonedWindowsStillOpenForConnectionList(aConnectionDefinitionList))
            {
                // Some clone-windows opened message 

                string confirmMessage = Properties.Resources.RemoveMultipleSystemsMessage; 
                confirmMessage += "\n\n" + Properties.Resources.ClonedWindowsStillOpenForSystemsMessage;

                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo,
                    MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2) != DialogResult.Yes)
                {
                    return;
                }
                // To Close all ManagedWindows associated with EACH  connection def.
                // Loop over the list of connection definitions
                foreach (ConnectionDefinition theConnectionDefinition in aConnectionDefinitionList)
                {
                    WindowsManager.CloseAllManagedWindowsPerConnection(theConnectionDefinition, true);

                    // Remove one connection definition
                    theConnectionDefinition.Remove(this);
                }
            }
            else
            {
                // No clone-windows opened
                string confirmMessage = Properties.Resources.RemoveMultipleSystemsMessage;

                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo,
                    MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2) != DialogResult.Yes)
                {
                    return;
                }
                // To Close all ManagedWindows associated with EACH  connection def.
                // Loop over the list of connection definitions
                foreach (ConnectionDefinition theConnectionDefinition in aConnectionDefinitionList)
                {
                    // Clear the password for one connection definition
                    theConnectionDefinition.Remove(this);
                }
            }

        } // DoRemoves

        /// <summary>
        /// Test all of the systems in a list
        /// </summary>
        /// <param name="aConnectionDefinitionList">the list of connection definitions</param>
        private void DoTests(List<ConnectionDefinition> aConnectionDefinitionList)
        {
            if (!Utilities.CheckOdbcDriverExists())
                return;
     
            // Loop over the list of connection definitions
            foreach (ConnectionDefinition theConnectionDefinition in aConnectionDefinitionList)
            {
                //Ignore not connected connection
                if (theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    // Test one connection definition
                    theConnectionDefinition.DoTestOnly();
                }
            }
        }

        /// <summary>
        /// Let the user edit a given connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">the given connection definition</param>
        private void DoEdit(ConnectionDefinition aConnectionDefinition)
        {
            /*Now we can edit a system when ODBC connected, but only Live Feed Properties*/
            // if connection is still active, user cannot edit the connection
            //if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            //{
            //    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ActiveSystemEditWarning, Properties.Resources.Warning, MessageBoxButtons.OK);
            //    return;
            //}


            WindowsManager.CloseAllManagedWindowsPerConnection(aConnectionDefinition, false);
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog(false);
            //bool enableLiveFeedConnection = TrafodionContext.Instance.TheTrafodionMain.ActiveAreaName.Contains("Monitoring");
            theConnectionDefinitionDialog.Edit(aConnectionDefinition);
            UpdateControls();
        }


        /// <summary>
        /// Let the user add a new connection definition like a given connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">the given connection definition</param>
        private void DoAddLike(ConnectionDefinition aConnectionDefinition)
        {

            // Create the dialog
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog();

            // Tell it to add a connection definition like the given connection definition
            theConnectionDefinitionDialog.NewLike(aConnectionDefinition);

        }

        /// <summary>
        /// Let the user add a new connection definition starting from scratch
        /// </summary>
        private void DoAddSystem()
        {

            // Create the dialog
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog();

            // Tell it to add a new connection definition
            theConnectionDefinitionDialog.New();
            if (theConnectionDefinitionDialog.DialogResult.Equals(DialogResult.OK))
            {
                UpdateControls();
            }

        }

        /// <summary>
        /// Clear the passwords for all of the systems in a list
        /// </summary>
        /// <param name="aConnectionDefinitionList">the list of connection definitions</param>
        private void DoClearPasswords(List<ConnectionDefinition> aConnectionDefinitionList)
        {
            // Check if some cloned windows are opened--display appropriate messsages e.g. 
            // Do you really want to clear the passwords for the selected system(s)?"
            // Close all clone-windows and, then only clear the password.  
            if (WindowsManager.ClonedWindowsStillOpenForConnectionList(aConnectionDefinitionList))
            {
                // Some clone-windows opened message 

                string confirmMessage =  Properties.Resources.ClearMultiplePasswordsMessage;
                confirmMessage += "\n\n" + Properties.Resources.ClonedWindowsStillOpenForSystemsMessage;

                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo) != DialogResult.Yes)
                {
                    return;
                }
                // To Close all ManagedWindows associated with EACH  connection def.
                // Loop over the list of connection definitions
                foreach (ConnectionDefinition theConnectionDefinition in aConnectionDefinitionList)
                {
                    WindowsManager.CloseAllManagedWindowsPerConnection(theConnectionDefinition, true);

                    // Clear the password for one connection definition
                    theConnectionDefinition.ClearPassword();
                }
            }
            else
            {
                // No clone-windows opened
                string confirmMessage = Properties.Resources.ClearMultiplePasswordsMessage;

                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo) != DialogResult.Yes)
                {
                    return;
                }
                // To Close all ManagedWindows associated with EACH  connection def.
                // Loop over the list of connection definitions
                foreach (ConnectionDefinition theConnectionDefinition in aConnectionDefinitionList)
                {
                    // Clear the password for one connection definition
                    theConnectionDefinition.ClearPassword();
                }
            }

        } // DoClearPasswords



        /// <summary>
        /// The user has clicked the Test button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheTestButtonClick(object sender, EventArgs e)
        {

            // Test all of the selected connection definitions.  It is assumed that the Test
            // button would be enabled only if there are one or more one systems selected.
            DoTests(_theMySystemsDataGridView.SelectedSystems);

        }

        /// <summary>
        /// The user has clicked the Edit button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheEditButtonClick(object sender, EventArgs e)
        {

            // Edit the selected system.  It is assumed that the Edit button would be enabled
            // only if there is exactly one system selected.
            DoEdit(_theMySystemsDataGridView.SelectedSystem);

        }

        /// <summary>
        /// The user has clicked the Add button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheAddButtonClick(object sender, EventArgs e)
        {

            // Add a new system from scratch
            DoAddSystem();

        }

        /// <summary>
        /// The user has clicked the Add Like button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheAddLikeButtonClick(object sender, EventArgs e)
        {

            // Add a system like the selected system.  It is assumed that the Add Like button 
            // would be enabled only if there is exactly one system selected.
            DoAddLike(_theMySystemsDataGridView.SelectedSystem);

        }

        /// <summary>
        /// The user has clicked the Remove button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheRemoveButtonClick(object sender, EventArgs e)
        {

            // Remove all of the selected connection definitions.  It is assumed that the Remove
            // button would be enabled only if there are one or more one systems selected.
            DoRemoves(_theMySystemsDataGridView.SelectedSystems);

        }

        /// <summary>
        /// The user has clicked the Disconnect button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheDisconnectButtonClick(object sender, EventArgs e)
        {

            // Clear the passwords for all of the selected connection definitions.  It is assumed
            // that the Disconnect button would be enabled only if there are one or
            // more one systems selected.
            DoClearPasswords(_theMySystemsDataGridView.SelectedSystems);
            UpdateControls();
        }
    }

    /// <summary>
    /// A user control that only shhows the active systems
    /// </summary>
    public class MyActiveSystemsUserControl : MySystemsUserControl
    {

        /// <summary>
        /// The constructor
        /// </summary>
        public MyActiveSystemsUserControl()
            : base(new MyActiveSystemsDataGridView())
        {
        }

    }

    /// <summary>
    /// A user control that only shhows the systems that are not active
    /// </summary>
    public class MyOtherSystemsUserControl : MySystemsUserControl
    {

        /// <summary>
        /// The constructor
        /// </summary>
        public MyOtherSystemsUserControl()
            : base(new MyOtherSystemsDataGridView())
        {
        }

    }

}
