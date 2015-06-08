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
using Trafodion.Manager.ConnectivityArea.Model;

using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;


namespace Trafodion.Manager.ConnectivityArea.Controls
{

    /// <summary>
    /// This user control allows the user to view, set, and edit the settings that define what
    /// the users thinks is a system but really is a connection definition.
    /// <para/>
    /// An event is fired anytime the user chnages anything in this control and a container can
    /// use this event to determine whether or not it should chnage its controls.
    /// </summary>
    public partial class ConnectivityAreaConfigManagementAddUserControl : UserControl
    {

        #region Member Variables

        // True if editing an existing connection definition, false if adding a new one
        private bool _isEditing = false;


        // The starting version for editing or adding like, null if new
        private object _theLoadedMgmtResource = null;

        #endregion

        /// <summary>
        /// The constructor
        /// </summary>
        public ConnectivityAreaConfigManagementAddUserControl()
        {

            // Call the designer-generated code
            InitializeComponent();

            this.mgmtAction_TrafodionComboBox.Items.AddRange(new object[] {
            new ActionItem(Trafodion.Manager.ConnectivityArea.Model.NDCSResource.ActionNames[1], 1),
            new ActionItem(Trafodion.Manager.ConnectivityArea.Model.NDCSResource.ActionNames[5], 5),
            new ActionItem(Trafodion.Manager.ConnectivityArea.Model.NDCSResource.ActionNames[2], 2)});

            this.mgmtAttribute_TrafodionComboBox.SelectedIndex = 0;
            this.mgmtAction_TrafodionComboBox.SelectedIndex = 0;
            this.mgmtLimit_TrafodionNumericUpDown.Maximum = long.MaxValue;
            this._theToolTip.SetToolTip(this.mgmtLimit_TrafodionNumericUpDown, "Please enter an integer from 0 to "+long.MaxValue);
        }

        /// <summary>
        /// ResourceObject
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public NDCSResource GetResourceObject()
        {
            NDCSResource newResource = new NDCSResource();
            newResource.AttributeName = mgmtAttribute_TrafodionComboBox.SelectedItem.ToString();
            //
            newResource.ActionID = ((ActionItem)this.mgmtAction_TrafodionComboBox.SelectedItem).ActionValue;
            //Use TryParse eventually
            newResource.Limit = Int64.Parse(this.mgmtLimit_TrafodionNumericUpDown.Value.ToString());
            return newResource;
        }


        /// <summary>
        /// Indicates that we are to edit an existing connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition to edit</param>
        public void EditFrom(NDCSResource aMgmtResource)
        {

            // We are editing
            _isEditing = true;

            // Load the settings into the controls
            LoadFrom(aMgmtResource);

            // the Connection Name is used as the key, it should not be changed once created.
            //_theConnectionNameTextBox.ReadOnly = true;

            // Set the focus in the password control
            //_thePasswordTextBox.Select();

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
                bool legal = true;//(theSameNameConnectionDefinition == null) || (_isEditing && (theSameNameConnectionDefinition == _theLoadedControlledTable));
                if (!legal)
                {
                    return "Please check your values and try again";//Properties.Resources.You_must_specify_a_name_that_is_not_already_in_use;
                }
            }

            return null;
        }

        /// <summary>
        /// Handle a chnage to any of the settings in this user control
        /// </summary>
        /// <param name="aConnectivityAreaConfigManagementAddUserControl">the user control that changed</param>
        public delegate void ChangedHandler(ConnectivityAreaConfigManagementAddUserControl aConnectivityAreaConfigManagementAddUserControl);

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
        private void LoadFrom(NDCSResource aMgmtResource)
        {

            // Remember it
            _theLoadedMgmtResource = aMgmtResource;

            int actionIndex = 0;
            int attributeIndex = 0;
            decimal limit = 0;

            actionIndex = Convert.ToInt32(aMgmtResource.ActionID);

            // Load the controls from the connection definition without firing events
            // by using the private variables directly
            //if (null != aMgmtResource.AttributeName)
            //this.mgmtAttribute_TrafodionComboBox.Items.IndexOf(aMgmtResource.AttributeName);

            //Since there is only 1 attribute so far
            attributeIndex = 0;

            if (aMgmtResource.Limit > this.mgmtLimit_TrafodionNumericUpDown.Maximum)
            { limit = this.mgmtLimit_TrafodionNumericUpDown.Maximum; }
            else if (aMgmtResource.Limit < this.mgmtLimit_TrafodionNumericUpDown.Minimum)
            { limit = this.mgmtLimit_TrafodionNumericUpDown.Minimum; }
            else
            { limit = aMgmtResource.Limit; }

            this.mgmtAttribute_TrafodionComboBox.SelectedIndex = attributeIndex;
            this.mgmtAction_TrafodionComboBox.SelectedItem = new ActionItem(NDCSResource.ActionNames[actionIndex], actionIndex);
            this.mgmtLimit_TrafodionNumericUpDown.Value = limit;

            // Load the drivers that are available and try to select the specified one
            //LoadTheDriverComboBox();//aControlledTable.DriverString);

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





    }

}
