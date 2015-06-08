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
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// This user control allows the user to view, set, and edit the settings that define what
    /// the users thinks is a system but really is a connection definition.
    /// <para/>
    /// An event is fired anytime the user chnages anything in this control and a container can
    /// use this event to determine whether or not it should chnage its controls.
    /// </summary>
    public partial class ConnectionDefinitionUserControl : UserControl
    {

        #region Member Variables

        // True if editing an existing connection definition, false if adding a new one
        private bool _isEditing = false;

        // The starting version for editing or adding like, null if new
        private ConnectionDefinition _theLoadedConnectionDefinition = null;
        //private static AutoCompleteStringCollection _defaultSchemaCache = new AutoCompleteStringCollection();
        //private static AutoCompleteStringCollection _roleNameCache = new AutoCompleteStringCollection();
        //private static readonly string _defaultSchemaCachePersistenceKey = "ConnectionControlDefaultSchemaCache";
        //private static readonly string _roleNameCachePersistenceKey = "ConnectionControlRoleNameCache";
        private int MAX_CACHE_SIZE = 10;
        // To distinguish an expltry password from one that has not been set
        private bool _passwordIsSet = false;

        #endregion

        /// <summary>
        /// The constructor
        /// </summary>
        public ConnectionDefinitionUserControl()
        {

            // Call the designer-generated code
            InitializeComponent();

            // Load the names of the installed HP ODBC drivers and try to select the default
            LoadTheDriverComboBox(Properties.Resources.DefaultTrafodionOdbcDriverString);

            // Always defualt the port number, will be overwritten if editing or adding like
            _thePortNumberTextBox.Text = ConnectionDefinition.TheDefaultPortNumber.ToString();

            // Always defualt the server datasource, will be overwritten if editing or adding like
            //_theServerDataSourceTextBox.Text = Properties.Resources.DefaultServerDataSourceName;

            // This parameterized tooltip cannot be set in the designer
            if (_theToolTip != null)
            {
                _theToolTip.SetToolTip(_thePortNumberTextBox, String.Format(Properties.Resources.A_port_number_in_the_range_0_to_1_the_default_is_2,
                               new object[] { ConnectionDefinition.TheMinPortNumber, ConnectionDefinition.TheMaxPortNumber, ConnectionDefinition.TheDefaultPortNumber }));
            }

            _theRoleNameTextBox.MaxLength = ConnectionDefinition.ROLE_NAME_MAX_LENGTH;
            _thePasswordTextBox.MaxLength = ConnectionDefinition.PASSWORD_MAX_LENGTH;
            _theUserNameTextBox.MaxLength = ConnectionDefinition.USER_NAME_MAX_LENGTH;

            LoadClientDSNCombo();
            LoadPersistence();

            //_theDefaultSchemaTextBox.AutoCompleteMode = AutoCompleteMode.Suggest;
            //_theDefaultSchemaTextBox.AutoCompleteSource = AutoCompleteSource.CustomSource;
            //_theDefaultSchemaTextBox.AutoCompleteCustomSource = _defaultSchemaCache;

            //_theRoleNameTextBox.AutoCompleteMode = AutoCompleteMode.Suggest;
            //_theRoleNameTextBox.AutoCompleteSource = AutoCompleteSource.CustomSource;
            //_theRoleNameTextBox.AutoCompleteCustomSource = _roleNameCache;

            _liveFeedPortNumberComboBox.SelectedIndex = 0;

        }

        //protected override bool ProcessDialogKey(Keys keyData)
        //{
        //    if(keyData.CompareTo(Keys.Enter) == 0 || keyData.CompareTo(Keys.Return) == 0)
        //    {
        //        if (_theRoleNameTextBox.Focused)
        //        {
        //            _theRoleNameTextBox.Select();
        //            return true;
        //        }
        //        else
        //        {
        //            if (_theDefaultSchemaTextBox.Focused)
        //            {
        //                _theDefaultSchemaTextBox.Select();
        //                return true;
        //            }
        //        }
        //    }
        //    return base.ProcessDialogKey(keyData);
        //}

        static void LoadPersistence()
        {
            //try
            //{
            //    _defaultSchemaCache = Persistence.Get(_defaultSchemaCachePersistenceKey) as AutoCompleteStringCollection;
            //    if (_defaultSchemaCache == null)
            //    {
            //        _defaultSchemaCache = new AutoCompleteStringCollection();
            //    }
            //    _roleNameCache = Persistence.Get(_roleNameCachePersistenceKey) as AutoCompleteStringCollection;
            //    if (_roleNameCache == null)
            //    {
            //        _roleNameCache = new AutoCompleteStringCollection();
            //    }
            //}
            //catch (Exception ex)
            //{
            //}
        }

        void MyDispose()
        {
            //try
            //{
            //    Persistence.Put(_defaultSchemaCachePersistenceKey, _defaultSchemaCache);
            //    Persistence.Put(_roleNameCachePersistenceKey, _roleNameCache);
            //}
            //catch (Exception ex)
            //{
            //}
        }

        /// <summary>
        /// Indicates the source for an add like
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition to use to intialize the controls</param>
        public void CopyFrom(ConnectionDefinition aConnectionDefinition)
        {

            // We are not editing
            _isEditing = false;

            // Load the settings into the controls
            LoadFrom(aConnectionDefinition);

        }

        /// <summary>
        /// Indicates that we are to edit an existing connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition to edit</param>
        public void EditFrom(ConnectionDefinition aConnectionDefinition)
        {

            // We are editing
            _isEditing = true;

            // Load the settings into the controls
            LoadFrom(aConnectionDefinition);

            // the Connection Name is used as the key, it should not be changed once created.
            _theConnectionNameTextBox.ReadOnly = true;

            // Set the focus in the password control
            _thePasswordTextBox.Select();

        }

            /// <summary>
        /// Save the settings in our controls into a connection defintion
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public void SaveTo(ConnectionDefinition aConnectionDefinition)
        {
            try
            {
                aConnectionDefinition.SetProperty(ConnectionDefinition.LiveFeedOnlyConnectProperty, _liveCheckBox.Checked.ToString());

                // Don't bother anyone till we are done
                if (aConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded && aConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
                {
                    // Save the controls to the connection definition                    
                    aConnectionDefinition.SuppressEvents();
                    aConnectionDefinition.Name = ""; // To make sure we fire an event below

                    aConnectionDefinition.ClientDataSource = ClientDataSource;
                    if (!string.IsNullOrEmpty(ClientDataSource) && !(this._theClientDSCombo.Items.Contains(ClientDataSource)))
                    {
                        //update form with the persisted server-side datasource
                        this._theClientDSCombo.Items.Add(aConnectionDefinition.ClientDataSource);
                    }

                    aConnectionDefinition.UserName = UserName;
                    aConnectionDefinition.SetProperty(ConnectionDefinition.UserRoleName, RoleName);
                    aConnectionDefinition.SetProperty(ConnectionDefinition.CertificateFilePath, CertificateFullPath);
                    if (_passwordIsSet)
                    {
                        // The definition has a specific, possibly empty, password
                        //Now we have LiveFeedConnectedState, in this case, it also needs password in M9, don't clear it when already live feed connected
                        if (aConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
                            aConnectionDefinition.Password = Password;                        
                    }
                    else
                    {
                        // The definition has no password
                        aConnectionDefinition.ClearPassword();
                    }

                    aConnectionDefinition.Host = Host;
                    aConnectionDefinition.Port = Port;
                    aConnectionDefinition.DefaultSchema = DefaultSchema;
                    aConnectionDefinition.DriverString = DriverString;
                    aConnectionDefinition.Name = ConnectionName;
                    aConnectionDefinition.SetProperty(ConnectionDefinition.LiveFeedPortProperty, LiveFeedPort);
                    aConnectionDefinition.SetProperty(ConnectionDefinition.LiveFeedRetryTimerProperty, LiveFeedRetryTimer);
                }
                else 
                {
                    if (aConnectionDefinition.LiveFeedPort != LiveFeedPort || aConnectionDefinition.LiveFeedRetryTimer!=LiveFeedRetryTimer) 
                    {
                        aConnectionDefinition.SetProperty(ConnectionDefinition.LiveFeedPortProperty, LiveFeedPort);
                        aConnectionDefinition.SetProperty(ConnectionDefinition.LiveFeedRetryTimerProperty, LiveFeedRetryTimer);
                        aConnectionDefinition.FireOnLiveFeedPropertyChanged(ConnectionDefinition.Reason.LiveFeedPort); //Use LiveFeedPort change reason as a representative
                    }
                    //ODBC Properties Change After Live Feed Connected
                    if (aConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded) 
                    {
                        aConnectionDefinition.SuppressEvents();
                        aConnectionDefinition.Port = Port;
                        aConnectionDefinition.DefaultSchema = DefaultSchema;
                        aConnectionDefinition.DriverString = DriverString;
                        aConnectionDefinition.ClientDataSource = ClientDataSource;
                    }
                }
            }
            finally
            {
                aConnectionDefinition.AllowEvents();
            }
        }

        public TrafodionCheckBox LiveFeedOnlyConnectButton
        {
            get { return _liveCheckBox; }
        }
        public TrafodionGroupBox ODBCConnectionProperties
        {
            get { return _theODBCConnectionProperties; }
        }

        public TrafodionGroupBox LiveFeedConnectionProperties
        {
            get { return _theLiveFeedConnectionProperties; }
        }

        public TrafodionGroupBox CommonConnectionProperties
        {
            get { return _theCommonConnectionProperties; }
        }

        /// <summary>
        /// Accessor for the user's name for this connection definition in its control
        /// </summary>
        public string ConnectionName
        {
            get { return _theConnectionNameTextBox.Text.Trim(); }
        }

        public string ClientDataSource
        {
            get 
            {
                if (this._theClientDSCombo.Text == null)
                {
                    return "";
                }

                string dsnName = this._theClientDSCombo.Text.ToString().Trim();
                if (dsnName.Length == 0 || dsnName.Equals("<Default DataSource>"))
                {
                    dsnName = "";
                }
                return dsnName; 
            }
        }

        /// <summary>
        /// Accessor for the user name in its control
        /// </summary>
        public string UserName
        {
            get { return _theUserNameTextBox.Text.Trim(); }
        }

        /// <summary>
        /// Accessor for the role name in its control
        /// </summary>
        public string RoleName
        {
            get { return _theRoleNameTextBox.Text.Trim(); }
        }

        /// <summary>
        /// Accessor for the password in its control
        /// </summary>
        public string Password
        {
            get { return _thePasswordTextBox.Text.Trim(); }
        }

        /// <summary>
        /// The PasswordTextBox object for the parent to subscribe to the Enter event.
        /// </summary>
        public TrafodionTextBox PasswordTextBox
        {
            get { return _thePasswordTextBox; }
        }

        /// <summary>
        /// Accessor for the host DNS name or IP address in its control
        /// </summary>
        public string Host
        {
            get { return _theHostTextBox.Text.Trim(); }
        }

        /// <summary>
        /// Accessor for the port number string in its control
        /// </summary>
        public string Port
        {
            get { return _thePortNumberTextBox.Text.Trim(); }
        }

        /// <summary>
        /// Accessor for the live feed port number string in its control
        /// </summary>
        public string LiveFeedPort
        {
            get { return _liveFeedPortNumberComboBox.Text.Trim(); }
        }

        /// <summary>
        /// Accessor for the live feed retry timer string in its control
        /// </summary>
        public string LiveFeedRetryTimer 
        {
            get { return _theLiveFeedRetryTimerTextBox.Text.Trim(); }
        }

        /// <summary>
        /// Accessor for the default schema in its control
        /// </summary>
        public string DefaultSchema
        {
            get { return _theDefaultSchemaTextBox.Text.Trim(); }
        }

        public string CertificateFullPath
        {
            get { return _certFileTextBox.Text.Trim(); }
        }

        /// <summary>
        /// Accessor for the selected driver in its control
        /// </summary>
        public string DriverString
        {
            get
            {
                object theSelectedItem = _theDriverComboBox.SelectedItem;

                if ((theSelectedItem == null) || !(theSelectedItem is DriverWrapper))
                {
                    return "";
                }
                return theSelectedItem.ToString();
            }
        }

        /// <summary>
        /// Check to see if the currently settings are legal.  This means that the connection name must
        /// be present and must not be the same as some other existing connection and that the port number
        /// is present and is an integer in the legal range.
        /// <para/>
        /// Any other settings are allowed to be missing or incorrect and the user can edit them later.
        /// </summary>
        /// <returns>null if valid else an error message string</returns>
        public string ValidateInfo()
        {

            // Check to see if the name is empty
            if ((ConnectionName == null) || (ConnectionName.Length == 0))
            {
                return Properties.Resources.You_must_specify_a_name;
            }
            //Check to see if the name is only consist of letters, digits, and the underscore character 
            else if (!System.Text.RegularExpressions.Regex.IsMatch(ConnectionName, @"^[\w.-]+$"))
            {
                return Properties.Resources.System_name_can_only_contain;
            }

            // Check to see if the name is in use for a ConnectionDefinition other than this one
            {

                // Check to see if there is already a connection with this name
                ConnectionDefinition theSameNameConnectionDefinition = ConnectionDefinition.Find(ConnectionName);

                // The name is legal if there is no such system or we are editing that particluar system.
                bool legal = (theSameNameConnectionDefinition == null) || (_isEditing && (theSameNameConnectionDefinition == _theLoadedConnectionDefinition));

                if (!legal)
                {
                    return Properties.Resources.You_must_specify_a_name_that_is_not_already_in_use;
                }
            }
            int thePortNumber = 0;
            // Check to see if the port number is sensible
            {
                string thePort = Port;

                // Make sure it's not empty
                if ((thePort == null) || (thePort.Length == 0))
                {
                    return Properties.Resources.You_must_specify_a_port_number;
                }

                // Make sure it's an integer and in range.  The converter will throw an exception
                // if it's not an integer and we will range check it if it is.
                TypeConverter theTypeConverter = TypeDescriptor.GetConverter(new int());
                try
                {
                    thePortNumber = (int)theTypeConverter.ConvertFromInvariantString(thePort);

                    // if we get here it is an integer so we range check it
                    if ((thePortNumber < ConnectionDefinition.TheMinPortNumber) || (thePortNumber > ConnectionDefinition.TheMaxPortNumber))
                    {
                        return String.Format(Properties.Resources.You_must_specify_a_port_number_in_the_range_0_to_1,
                            new object[] { ConnectionDefinition.TheMinPortNumber, ConnectionDefinition.TheMaxPortNumber });
                    }
                }
                catch (Exception e)
                {

                    // Exception means it's not an integer
                    return Properties.Resources.You_must_specify_a_numeric_ODBC_port_number;

                }
                
            }

            // Check to see if the live feed port number is sensible
            {
                string theLiveFeedPort = LiveFeedPort;

                // Make sure it's not empty
                if ((theLiveFeedPort == null) || (theLiveFeedPort.Length == 0))
                {
                    return Properties.Resources.You_must_specify_a_live_feed_port_number;
                }
                int theLFPortNumber = 0;
                // Make sure it's an integer and in range.  The converter will throw an exception
                // if it's not an integer and we will range check it if it is.
                TypeConverter theTypeConverter = TypeDescriptor.GetConverter(new int());
                try
                {
                    theLFPortNumber = (int)theTypeConverter.ConvertFromInvariantString(theLiveFeedPort);

                    // if we get here it is an integer so we range check it
                    if ((theLFPortNumber < ConnectionDefinition.TheMinPortNumber) || (theLFPortNumber > ConnectionDefinition.TheMaxPortNumber))
                    {
                        return String.Format(Properties.Resources.You_must_specify_a_live_feed_port_number_in_the_range_0_to_1,
                            new object[] { ConnectionDefinition.TheMinPortNumber, ConnectionDefinition.TheMaxPortNumber });
                    }
                }
                catch (Exception e)
                {
                    if (theLiveFeedPort != "Default Port Number")
                    {
                        // Exception means it's not an integer
                        return Properties.Resources.You_must_specify_a_numeric_live_feed_port_number;
                    }
                }

                //check to see if ODBC port number is same with live feed port number
                if (thePortNumber == theLFPortNumber) return Properties.Resources.Cant_use_same_port_number;

            }

            // Check to see if the live feed retry timer is sensible
            {

                if (!string.IsNullOrEmpty(_theLiveFeedRetryTimerTextBox.Text))
                {

                    // Make sure it's an integer and in range.  The converter will throw an exception
                    // if it's not an integer and we will range check it if it is.
                    TypeConverter theTypeConverter = TypeDescriptor.GetConverter(new int());
                    try
                    {
                        int timer = (int)theTypeConverter.ConvertFromInvariantString(_theLiveFeedRetryTimerTextBox.Text);
                    }
                    catch (Exception e)
                    {
                        // Exception means it's not an integer
                        return string.Format(Properties.Resources.You_must_specify_a_numeric_value, "Live Feed Retry Timer");
                    }
                }

            }




            //Check to see if certificate is valid
            string certFileName = _certFileTextBox.Text.Trim();
            if (!string.IsNullOrEmpty(certFileName))
            {
                if (!File.Exists(certFileName))
                {
                    return "The specified certificate file does not exist";
                }
            }

            // Name is unique and port is legal
            return null;

        }

        /// <summary>
        /// Handle a chnage to any of the settings in this user control
        /// </summary>
        /// <param name="aConnectionDefinitionUserControl">the user control that changed</param>
        public delegate void ChangedHandler(ConnectionDefinitionUserControl aConnectionDefinitionUserControl);

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
        private void LoadFrom(ConnectionDefinition aConnectionDefinition)
        {

            // Remember it
            _theLoadedConnectionDefinition = aConnectionDefinition;

            //get the user provided role name from the property
            string userProvidedRole = _theLoadedConnectionDefinition.UserSpecifiedRole;
            userProvidedRole = (userProvidedRole == null) ? "" : userProvidedRole;

            if (!string.IsNullOrEmpty(aConnectionDefinition.ClientDataSource) && !(this._theClientDSCombo.Items.Contains(aConnectionDefinition.ClientDataSource)))
            {
                //update form with the persisted server-side datasource
                this._theClientDSCombo.SelectedIndex = this._theClientDSCombo.Items.Add(aConnectionDefinition.ClientDataSource);
            }
            
            // Load the controls from the connection definition without firing events
            // by using the rpivate variables directly
            _theConnectionNameTextBox.Text = aConnectionDefinition.Name;
            _theUserNameTextBox.Text = aConnectionDefinition.UserName;
            _theRoleNameTextBox.Text = userProvidedRole;
            _thePasswordTextBox.Text = aConnectionDefinition.Password;
            _theHostTextBox.Text = aConnectionDefinition.Host;
            _thePortNumberTextBox.Text = aConnectionDefinition.Port;
            _theDefaultSchemaTextBox.Text = aConnectionDefinition.DefaultSchema;
            _certFileTextBox.Text = aConnectionDefinition.CertificateFileFullPath;

            if (aConnectionDefinition.LiveFeedPort == "Default Port Number" ||
                aConnectionDefinition.LiveFeedPort == "-1")
            {
                _liveFeedPortNumberComboBox.SelectedIndex = 0;
            }
            else
            {
                _liveFeedPortNumberComboBox.SelectedIndex = -1;
                _liveFeedPortNumberComboBox.Text = aConnectionDefinition.LiveFeedPort;
            }
            _theLiveFeedRetryTimerTextBox.Text = aConnectionDefinition.LiveFeedRetryTimer;

            // Load the drivers that are available and try to select the specified one
            LoadTheDriverComboBox(aConnectionDefinition.DriverString);

            //Select the client DSN if that is available
            if (string.IsNullOrEmpty(aConnectionDefinition.ClientDataSource))
            {
                //no datasource selected/persisted
                this._theClientDSCombo.SelectedItem = 0;
            }
            else if (this._theClientDSCombo.Items.Contains(aConnectionDefinition.ClientDataSource))
            {
                //update form with the persised client-side datasource
                this._theClientDSCombo.SelectedItem = aConnectionDefinition.ClientDataSource;
            }
        }

        /// <summary>
        /// The contents of the ConnectionNameTextBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheConnectionNameTextBoxTextChanged(object sender, EventArgs e)
        {

            // Make the controls consistent
            UpdateControls();

        }

        /// <summary>
        /// The contents of the UserIDTextBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheUserIDTextBoxTextChanged(object sender, EventArgs e)
        {

            // Make the controls consistent
            UpdateControls();

        }

        /// <summary>
        /// The contents of the ThePasswordTextBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ThePasswordTextBoxTextChanged(object sender, EventArgs e)
        {
            _passwordIsSet = true;

            // Make the controls consistent
            UpdateControls();

        }

        /// <summary>
        /// The contents of the TheHostTextBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheHostTextBoxTextChanged(object sender, EventArgs e)
        {

            // Make the controls consistent
            UpdateControls();

        }

        /// <summary>
        /// The contents of the ThePortNumberTextBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ThePortNumberTextBoxTextChanged(object sender, EventArgs e)
        {

            // Make the controls consistent
            UpdateControls();

        }

        /// <summary>
        /// The contents of the SchemaTextBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheSchemaTextBoxTextChanged(object sender, EventArgs e)
        {

            // Make the controls consistent
            UpdateControls();

        }

        /// <summary>
        /// The selection in the DriverComboBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheDriverComboBoxSelectedIndexChanged(object sender, EventArgs e)
        {

            // Make the controls consistent
            UpdateControls();

        }

        /// <summary>
        /// The context of the LiveFeedPortComboBox changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheLiveFeedPortNumberComboBoxTextChanged(object sender, EventArgs e)
        {

            // Make the controls consistent
            UpdateControls();

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
        /// The list of listeners for our events
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
        /// Called when the user clicks the Clear password button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheClearPasswordButtonClick(object sender, EventArgs e)
        {
            // Must clear the password first
            _thePasswordTextBox.Text = "";

            // And then clear the flag because the changed handler sets the flag
            _passwordIsSet = false;

            // And make the controls consitent
            UpdateControls();

        }

        private void LoadTheDriverComboBox(string aDriverString)
        {
            _theDriverComboBox.Items.Clear();

            List<string> theValueNames = Utilities.GetHpTrafodionOdbcDriverNames();
            foreach (string theValueName in theValueNames)
            {
                if (!theValueName.StartsWith("HP ODBC 2"))//Ignore HP ODBC 2.0 for TrafodionManager 3.0.
                {
                    _theDriverComboBox.Items.Add(new DriverWrapper(theValueName));
                }
            }

            if (_theDriverComboBox.Items.Count == 0)
            {
//                NoHpOdbcDriversDialog theNoHpOdbcDriversDialog = new NoHpOdbcDriversDialog();
//                theNoHpOdbcDriversDialog.ShowDialog();
            }
            else
            {
                _theDriverComboBox.SelectedIndex = _theDriverComboBox.FindStringExact(aDriverString);
            }

        }

        private void LoadClientDSNCombo ()
        {
            List<String> odbcDataSourcesList = null;

            try
            {
                odbcDataSourcesList = OdbcAccess.getOdbcDataSources();

                //For testing only
                //if (odbcDataSourcesList.Count < 1)
                //{
                //    MessageBox.Show("\nWarning : No HP ODBC DataSource have been configured on this machine.\n\n",
                //                    "No DataSources Configured Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                //}

            }
            catch (Exception ex)
            {
                MessageBox.Show("\nError : Error fetching DataSource configuration information.\n\n" +
                                "Problem:  \t Internal error getting the list of configured DataSources.\n\n" +
                                "Solution: \t Please see error details for recovery.\n\n" +
                                "Details:  \t " + ex.Message + "\n\n", "DataSource List Fetch Error",
                                MessageBoxButtons.OK, MessageBoxIcon.Error);

            }

            String currentDSN = null;

            _theClientDSCombo.Items.Add("<Default DataSource>");

            if (odbcDataSourcesList != null)
            {
                foreach (string s in odbcDataSourcesList)
                {
                    int idx = _theClientDSCombo.Items.Add(s);
                }
            }


            if (null != currentDSN)
            {
                for (int idx = 0; idx < _theClientDSCombo.Items.Count; idx++)
                {
                    String dsn = (String)_theClientDSCombo.Items[idx].ToString();
                    if (currentDSN.Equals(dsn.ToUpper()))
                        _theClientDSCombo.SelectedIndex = idx;
                }
            }
            else
                _theClientDSCombo.SelectedIndex = 0;


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

        private void _theClientDSCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox cb = (ComboBox)sender;
            int index = cb.SelectedIndex;
            string _driverVersion = "";
            string _serverName = "";
            string _portNumber = "";

            if (index != 0)
            {
                string dsName = (string)cb.SelectedItem;
                String serverInfo = Utilities.getODBCStringValue(dsName, Utilities.REGISTRY_SERVER_KEY);
                _driverVersion = Utilities.getODBCDriverVersion(dsName);
                _serverName = OdbcAccess.getHostNameFromServer(serverInfo).Trim();
                this._theHostTextBox.Text = _serverName;
                _portNumber = OdbcAccess.getPortNumberFromServer(serverInfo).Trim();
                this._thePortNumberTextBox.Text = _portNumber;
                this._theDefaultSchemaTextBox.Text = Utilities.getODBCStringValue(dsName, Utilities.REGISTRY_SCHEMA_KEY);
    
            }
        }

        private void _certBrowseButton_Click(object sender, EventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.AddExtension = true;
            openFileDialog.DefaultExt = "cer";
            openFileDialog.Filter = "Certificate File (*.cer)|*.cer";
            openFileDialog.InitialDirectory = ConnectionDefinition.DefaultCertificateDirectory;
            openFileDialog.Title = "Select Certificate File";

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                _certFileTextBox.Text = openFileDialog.FileName;
            }
            openFileDialog.Dispose();
        }
       
    }
}
