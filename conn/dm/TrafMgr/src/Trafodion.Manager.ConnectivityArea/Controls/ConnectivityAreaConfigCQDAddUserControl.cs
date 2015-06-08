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
    public partial class ConnectivityAreaConfigCQDAddUserControl : UserControl
    {

        #region Member Variables

        // True if editing an existing connection definition, false if adding a new one
        private bool _isEditing = false;
        private TrafodionButton _OKButton = null;

        // The starting version for editing or adding like, null if new
        private CQD _theLoadedCQD = null;

        #endregion
        public TrafodionButton OKButton
        {
            set
            {
                _OKButton = value;
                HandleOKButton();
            }
        }

        public CQD CQD
        {
            get {
                CQD newCQD = new CQD();
                newCQD.Attribute = this.CQDName_TrafodionTextBox.Text;
                newCQD.Value = this.CQDValue_TrafodionTextBox.Text;
                return newCQD;
            }
        }
        /// <summary>
        /// The constructor
        /// </summary>
        public ConnectivityAreaConfigCQDAddUserControl()
        {

            // Call the designer-generated code
            InitializeComponent();

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

         /// <summary>
        /// Check to see if the currently settings are legal.  
        /// <para/>
        /// Any other settings are allowed to be missing or incorrect and the user can edit them later.
        /// </summary>
        /// <returns>null if valid else an error message string</returns>
        public string ValidateInfo()
        {

            {
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
        /// <param name="aConnectivityAreaConfigCQDAddUserControl">the user control that changed</param>
        public delegate void ChangedHandler(ConnectivityAreaConfigCQDAddUserControl aConnectivityAreaConfigCQDAddUserControl);

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
        public void EditFrom(CQD aCQD)
        {

            // Remember it
            _theLoadedCQD = aCQD;
            //Disable the Name field
            this.CQDName_TrafodionTextBox.ReadOnly = true;
            this.CQDName_TrafodionTextBox.Text = _theLoadedCQD.Attribute;
            this.CQDValue_TrafodionTextBox.Text = _theLoadedCQD.Value;
            this.CQDValue_TrafodionTextBox.Select();
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

        private void HandleOKButton()
        {
            if (null != this._OKButton)
                this._OKButton.Enabled = (this.CQDValue_TrafodionTextBox.Text.Length > 0 && this.CQDName_TrafodionTextBox.Text.Length > 0);
        }

        private void CQDValue_TrafodionTextBox_TextChanged(object sender, EventArgs e)
        {
            HandleOKButton();
        }

        private void CQDName_TrafodionTextBox_TextChanged(object sender, EventArgs e)
        {
            HandleOKButton();
        }





    }

}
