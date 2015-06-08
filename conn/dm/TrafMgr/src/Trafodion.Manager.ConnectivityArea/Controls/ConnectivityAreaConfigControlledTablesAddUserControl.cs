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
using System.ComponentModel;
using System.Windows.Forms;

using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;
namespace Trafodion.Manager.ConnectivityArea.Controls
{

    /// <summary>
    /// This user control allows the user to view, set, and edit the settings that define what
    /// the users thinks is a system but really is a connection definition.
    /// <para/>
    /// An event is fired anytime the user chnages anything in this control and a container can
    /// use this event to determine whether or not it should chnage its controls.
    /// </summary>
    public partial class ConnectivityAreaConfigControlledTablesAddUserControl : UserControl
    {

        #region Member Variables

        // True if editing an existing connection definition, false if adding a new one
        private bool _isEditing = false;

        // The starting version for editing or adding like, null if new
        private ControlledTable _theLoadedControlledTable = null;
        private Trafodion.Manager.ConnectivityArea.Model.ControlledTable controlledTable;
        private ConnectivityAreaConfigControlledTablesAddBrowseDialog browseDialog = null;

        private TrafodionButton _OKButton = null;
        public TrafodionButton OKButton
        {
            set { 
                _OKButton = value;
                this.ValidateControlledTable();
            }
        }
        #endregion

        public Trafodion.Manager.ConnectivityArea.Model.ControlledTable ControlledTable
        {
            get {

                this.controlledTable = new Trafodion.Manager.ConnectivityArea.Model.ControlledTable(this.ctName_TrafodionTextBox.Text);

                this.controlledTable.Name = this.ctName_TrafodionTextBox.Text;

                controlledTable.IfLocked = "";
                if (this.ctIfLocked_TrafodionComboBox.SelectedIndex > 0)
                    controlledTable.IfLocked = this.ctIfLocked_TrafodionComboBox.SelectedItem.ToString();

                controlledTable.Mdam = "";
                if (this.ctMDAM_TrafodionComboBox.SelectedIndex > 0)
                    controlledTable.Mdam = this.ctMDAM_TrafodionComboBox.SelectedItem.ToString();

                if (this.ctPriority_TrafodionCheckBox.Checked)
                    controlledTable.Priority = this.ctPriority_TrafodionTrackBar.Value;

                controlledTable.SimilarityCheck = "";
                if (this.ctSimilarityCheck_TrafodionComboBox.SelectedIndex > 0)
                    controlledTable.SimilarityCheck = this.ctSimilarityCheck_TrafodionComboBox.SelectedItem.ToString();

                controlledTable.TableLock = "";
                if (this.ctTableLock_TrafodionComboBox.SelectedIndex > 0)
                    controlledTable.TableLock = this.ctTableLock_TrafodionComboBox.SelectedItem.ToString();

                if (this.ctTimeoutOptions_TrafodionRadioButton.Checked)
                    controlledTable.Timeout = -1;

                controlledTable.Timeout = Double.NaN; 
                if (this.ctTimeoutOptions_TrafodionRadioButton.Checked)
                {
                    if (this.ctTimeoutOptions_TrafodionComboBox.SelectedIndex == 0)
                    {
                        controlledTable.Timeout = ControlledTable.UNDEFINED_TIMEOUT;
                    }
                    else if (this.ctTimeoutOptions_TrafodionComboBox.SelectedIndex == 1)
                    {
                        controlledTable.Timeout = ControlledTable.NO_TIMEOUT; 
                    }
                    else
                    {
                        controlledTable.Timeout = ControlledTable.WILLNOTWAIT_TIMEOUT; 
                    }
                }
                else
                {
                    controlledTable.Timeout = (Double)this.ctTimeoutInterval_TrafodionNumericUpDown.Value; 
                }
                
                return controlledTable; 
            }
        }

        /// <summary>
        /// The constructor
        /// </summary>
        public ConnectivityAreaConfigControlledTablesAddUserControl()
        {

            // Call the designer-generated code
            InitializeComponent();
            SetupControls();
           
            // Load the names of the installed HP Trafodion ODBC drivers and try to select the default
            //LoadTheDriverComboBox(Properties.Resources.DefaultHpOdbcDriverString);

            // Always defualt the port number, will be overwritten if editing or adding like
            //_thePortNumberTextBox.Text = ConnectionDefinition.TheDefaultPortNumber.ToString();

            // Always defualt the server datasource, will be overwritten if editing or adding like
            //_theServerDataSourceTextBox.Text = Properties.Resources.DefaultServerDataSourceName;

            // This parameterized tooltip cannot be set in the designer
            //_theToolTip.SetToolTip(_thePortNumberTextBox, String.Format(Properties.Resources.A_port_number_in_the_range_0_to_1_the_default_is_2,
            //                new object[] { ConnectionDefinition.TheMinPortNumber, ConnectionDefinition.TheMaxPortNumber, ConnectionDefinition.TheDefaultPortNumber }));

        }

        private void SetupControls()
        {

            browseDialog = new ConnectivityAreaConfigControlledTablesAddBrowseDialog();

            this.ctIfLocked_TrafodionComboBox.SelectedIndex = 0;
            this.ctMDAM_TrafodionComboBox.SelectedIndex = 0;
            this.ctSimilarityCheck_TrafodionComboBox.SelectedIndex = 0;
            this.ctTableLock_TrafodionComboBox.SelectedIndex = 0;
            this.ctTimeoutOptions_TrafodionComboBox.SelectedIndex = 0;
            this.ctPriority_TrafodionTrackBar.Enabled = this.ctPriority_TrafodionCheckBox.Checked;

            this.ctTimeoutInterval_TrafodionNumericUpDown.Minimum = 1;
            this.ctTimeoutInterval_TrafodionNumericUpDown.Maximum = 2147483519;

            //Default value
            this.ctTimeoutOptions_TrafodionRadioButton.Checked = true; 

            this.addCT_TrafodionToolTip.SetToolTip(ctTimeoutInterval_TrafodionRadioButton, "Please enter an integer from 1 to 2,147,483,519.");
            this.addCT_TrafodionToolTip.SetToolTip(ctTimeoutInterval_TrafodionNumericUpDown, "Please enter an integer from 1 to 2,147,483,519.");
            this.addCT_TrafodionToolTip.SetToolTip(ctPriority_TrafodionCheckBox, "Click the checkbox and select a value to set the priority.");
            this.addCT_TrafodionToolTip.SetToolTip(ctPriority_TrafodionTrackBar, "Click the checkbox and select a value to set the priority.");
       
        }


        /// <summary>
        /// Indicates that we are to edit an existing connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition to edit</param>
        public void EditFrom(ControlledTable aControlledTable)
        {

            // We are editing
            _isEditing = true;

            // Load the settings into the controls
            LoadFrom(aControlledTable);
            this.ctName_TrafodionTextBox.ReadOnly = true;

            //disable the ok button until a change is made
            ValidateControlledTable(false);

        }

            /// <summary>
        /// Save the settings in our controls into a connection defintion
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public void SaveTo(object aControlledTable)
        {

            try
            {
                // Save the data stored in the controls to the controlled table
                // aControlledTable.Name = ""; // To make sure we fire an event below

            }
            finally
            {
            }

            // This will fire a single event
            //aControlledTable.Name = ConnectionName;

        }

        /// <summary>
        /// Accessor for the user's name for this connection definition in its control
        /// </summary>
        public string ConnectionName
        {
            get { return ""; }//_theConnectionNameTextBox.Text.Trim(); }
        }

        /// <summary>
        /// Check to see if the currently settings are legal.  
        /// <para/>
        /// Any other settings are allowed to be missing or incorrect and the user can edit them later.
        /// </summary>
        /// <returns>null if valid else an error message string</returns>
        public string ValidateInfo()
        {

            // Check to see if the name is empty
            /*if ((ConnectionName == null) || (ConnectionName.Length == 0))
            {
                return Properties.Resources.You_must_specify_a_name;
            }*/

            // Check to see if the name is in use for a Controlled Table other than this one
            {
                // Check to see if there is already a connection with this name
                //ControlledTable theSameNameControlledTable = ControlledTable.Find(ControlledTableName);

                // The name is legal if there is no other controlled table by this name.
                bool legal = !string.IsNullOrEmpty(this.ctName_TrafodionTextBox.Text.Trim());

                if (!legal)
                {
                    return "Table name is a required value";//Properties.Resources.You_must_specify_a_name_that_is_not_already_in_use;
                }
            }

            return null;
        }

        /// <summary>
        /// Handle a chnage to any of the settings in this user control
        /// </summary>
        /// <param name="aConnectivityAreaConfigControlledTablesAddUserControl">the user control that changed</param>
        public delegate void ChangedHandler(ConnectivityAreaConfigControlledTablesAddUserControl aConnectivityAreaConfigControlledTablesAddUserControl);

        /// <summary>
        /// Allow others to add or remove changed event listeners
        /// </summary>
        public event ChangedHandler Changed
        {
            add { _theEventHandlers.AddHandler(_theChangedKey, value); }
            remove { _theEventHandlers.RemoveHandler(_theChangedKey, value); }
        }

        /// <summary>
        /// Loads the settings from a connection defintion into our controls
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition to use to intialize the controls</param>
        private void LoadFrom(ControlledTable aControlledTable)
        {

            // Remember it
            _theLoadedControlledTable = aControlledTable;

            this.ctName_TrafodionTextBox.Text = _theLoadedControlledTable.Name;

            if (_theLoadedControlledTable.IfLocked != "")
                this.ctIfLocked_TrafodionComboBox.SelectedItem = _theLoadedControlledTable.IfLocked;

            if (_theLoadedControlledTable.Mdam != "")
                this.ctMDAM_TrafodionComboBox.SelectedItem = _theLoadedControlledTable.Mdam;

            if (_theLoadedControlledTable.Priority > 0)
            {
                this.ctPriority_TrafodionCheckBox.Checked = true;
                this.ctPriority_TrafodionTrackBar.Enabled = true;
                this.ctPriority_TrafodionTrackBar.Value = _theLoadedControlledTable.Priority;
            }

            if (_theLoadedControlledTable.SimilarityCheck != "")
                this.ctSimilarityCheck_TrafodionComboBox.SelectedItem = _theLoadedControlledTable.SimilarityCheck;

            if (_theLoadedControlledTable.TableLock != "")
                this.ctTableLock_TrafodionComboBox.SelectedItem = _theLoadedControlledTable.TableLock;


            //if (_theLoadedControlledTable.Timeout == -1)
            //    this.ctTimeoutOptions_TrafodionRadioButton.Checked = true;

            if (_theLoadedControlledTable.Timeout == Double.NaN)
                   {
                       //Handle NaN
                   }
            else if (_theLoadedControlledTable.Timeout == ControlledTable.NO_TIMEOUT)
                {   
                    this.ctTimeoutOptions_TrafodionRadioButton.Checked = true;
                    this.ctTimeoutOptions_TrafodionComboBox.SelectedIndex = 1;
                }
            else if (_theLoadedControlledTable.Timeout == ControlledTable.WILLNOTWAIT_TIMEOUT)
            {
                this.ctTimeoutOptions_TrafodionRadioButton.Checked = true;
                this.ctTimeoutOptions_TrafodionComboBox.SelectedIndex = 2;
            }
            else if (_theLoadedControlledTable.Timeout == ControlledTable.UNDEFINED_TIMEOUT)
            {
                this.ctTimeoutOptions_TrafodionRadioButton.Checked = true;
                this.ctTimeoutOptions_TrafodionComboBox.SelectedIndex = 0;
            }
            else
            {
                this.ctTimeoutInterval_TrafodionRadioButton.Checked = true;
                this.ctTimeoutInterval_TrafodionNumericUpDown.Value = Convert.ToInt32(_theLoadedControlledTable.Timeout);
            }

        }


        /// <summary>
        /// Make the controls consistent
        /// </summary>
        private void UpdateControls()
        {

            // We don't have any controls to keep consistent but someone else may so tell them
            FireChanged();

        }

        /// <summary>
        /// The list of listernres for our events
        /// </summary>
        static private EventHandlerList _theEventHandlers = new EventHandlerList();

        /// <summary>
        /// The key for handlers of our chaged event
        /// </summary>
        private static readonly string _theChangedKey = "Changed";

        /// <summary>
        /// Notify all listeners that we have chnaged
        /// </summary>
        private void FireChanged()
        {

            // Get all of the listeners to the changed event
            ChangedHandler theChangedHandlers = (ChangedHandler)_theEventHandlers[_theChangedKey];

            // Check to see if there are any
            if (theChangedHandlers != null)
            {

                // Multicast to them
                theChangedHandlers(this);

            }
        }

   
        /// <summary>
        /// This class will let us keep info about each driver in the ComboBox.
        /// </summary>
        private class DriverWrapper : IComparable
        {
            private string _theName;

            internal DriverWrapper(string aName)
            {
                _theName = aName;
            }

            override public string ToString()
            {
                return _theName;
            }


            #region IComparable Members

            public int CompareTo(object anObject)
            {
                return anObject.ToString().CompareTo(ToString());
            }

            #endregion
        }

        private void browse_TrafodionButton_Click(object sender, EventArgs e)
        {
 
            browseDialog.OpenDialog();
            if(browseDialog.DialogResult == DialogResult.OK)
                this.ctName_TrafodionTextBox.Text = browseDialog.BrowseUserControl.GetFullPathOfSelected();

        }


        private void ValidateControlledTable()
        {
            ValidateControlledTable(false);
        }

        private void ValidateControlledTable(bool changedValue)
        {
            //This method toggles the "OK" button based on a valid name and timeout option.
            
            if (null == _OKButton)
                return;

            /*
            if (this.ctName_TrafodionTextBox.Text.Length <= 0)
            {
                _OKButton.Enabled = false;
                return;
            }

            if (this._isEditing)
            {
                _OKButton.Enabled = changedValue;
                return;
            }

            //Check for changes from the default values. For new Controlled Tables only.
            if (
                (this.ctTimeoutOptions_TrafodionRadioButton.Checked && this.ctTimeoutOptions_TrafodionComboBox.SelectedIndex == 0)
                && this.ctTableLock_TrafodionComboBox.SelectedIndex == 0
                && this.ctSimilarityCheck_TrafodionComboBox.SelectedIndex == 0
                && this.ctPriority_TrafodionCheckBox.Checked == false
                && this.ctIfLocked_TrafodionComboBox.SelectedIndex == 0
                && this.ctMDAM_TrafodionComboBox.SelectedIndex == 0                               
                )
            {
               _OKButton.Enabled = false;
               return;
            } 
            */

            _OKButton.Enabled = !string.IsNullOrEmpty(this.ctName_TrafodionTextBox.Text.Trim());
        }

        private void ctPriority_TrafodionCheckBox_Click(object sender, EventArgs e)
        {
            this.ctPriority_TrafodionTrackBar.Enabled = ctPriority_TrafodionCheckBox.Checked;
            ValidateControlledTable(true);
        }

        private void ctName_TrafodionTextBox_TextChanged(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctTimeoutOptions_TrafodionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctTimeoutInterval_TrafodionRadioButton_Click(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctTimeoutOptions_TrafodionRadioButton_Click(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctIfLocked_TrafodionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctMDAM_TrafodionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        void ctPriority_TrafodionTrackBar_Scroll(object sender, System.EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctTimeoutInterval_TrafodionNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctSimilarityCheck_TrafodionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        private void ctTableLock_TrafodionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ValidateControlledTable(true);
        }

        void ctPriority_TrafodionTrackBar_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            ValidateControlledTable(true);
        }




    }

}
