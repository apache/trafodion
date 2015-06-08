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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;
using System.Collections;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class ConfigureDirectoryServerDialog : TrafodionForm
    {

        #region Fields

        private ConnectionDefinition _theConnectionDefinition = null;
        private DirectoryServer _theDirectoryServer = null;
        private bool _theEditDialog = false;
        private int _intialDialogHeight = 0;
        private int _initialServerControlHeight = 0;
        private ConfigureDirectoryServerUserControl _theConfigureDirectoryServerUserControl = null;
        private ConfigureActiveDirectoryServerUserControl _theConfigureActiveDirectoryServerUserControl = null;
        private ArrayList _commonParameters = null;
        private short _origUpdatedAttrVector = 0;
        private bool _origCommonParameterUpdatedFlag = false;
        private bool _firstTime = true;
        private bool _initialized = false;
        private bool _errorEncountered = false;

        #endregion 

        #region Properties

        /// <summary>
        /// StatusMessage: the error message to be displayed.
        /// </summary>
        public string StatusMessage
        {
            get { return _theStatusTextBox.Text; }
            set 
            { 
                _theStatusTextBox.Text = value;
                if (String.IsNullOrEmpty(value))
                {
                    _theStatusTextBox.Visible = false;
                }
                else
                {
                    _theStatusTextBox.Visible = true;
                }
            }
        }

        /// <summary>
        /// Initialized: the dialog has been properly initialized.
        /// </summary>
        public bool Initialized
        {
            get { return _initialized; }
            set { _initialized = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor: ConfigureDirectoryServerDialog
        /// Dialog for configuring a directory server entry
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="isEditDialog"></param>
        public ConfigureDirectoryServerDialog(ConnectionDefinition aConnectionDefinition, bool isEditDialog)
        {
            _theConnectionDefinition = aConnectionDefinition;
            _theEditDialog = isEditDialog;

            InitializeComponent();
            CenterToParent();

            if (_theEditDialog)
            {
                this.Text = String.Format("{0}{1}", TitlePrefix, Properties.Resources.EditServer);
                this._theAddButton.Enabled = true;
                this._serverTypeComboBox.Enabled = false;
            }

            // Adjust the height accordingly. 
            _initialServerControlHeight = _directoryServerPanel.Height;
            _intialDialogHeight = Height;
            _serverTypeComboBox.SelectedIndex = 0;

            // Register to the disposed event in case we need to clean up anything.
            this.Disposed += new EventHandler(ConfigureDirectoryServerDialog_Disposed);
            if (!_errorEncountered)
            {
                Initialized = true;
            }
        }

        /// <summary>
        /// Constructor: ConfigureDirectoryServerDialog
        /// Dialog for configuring a directory server entry.
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aDirectoryServer"></param>
        /// <param name="anEditDialog"></param>
        public ConfigureDirectoryServerDialog(ConnectionDefinition aConnectionDefinition, DirectoryServer aDirectoryServer, bool isEditDialog)
            : this(aConnectionDefinition, isEditDialog)
        {
            _theDirectoryServer = aDirectoryServer;
            LoadAttributes(_theDirectoryServer, false);
            if (_errorEncountered)
            {
                Initialized = false;
            }
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Disposed event, clean up registered event handlers. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ConfigureDirectoryServerDialog_Disposed(object sender, EventArgs e)
        {
            if (null != _theConfigureDirectoryServerUserControl)
            {
                _theConfigureDirectoryServerUserControl.OnRequiredFieldsChanged -= HandleRequiredFieldsChanged;
            }
            
            if (null != _theConfigureActiveDirectoryServerUserControl)
            {
                _theConfigureActiveDirectoryServerUserControl.OnRequiredFieldsChanged -= HandleRequiredFieldsChanged;
            }

        }

        /// <summary>
        /// Event handler for required fields changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void HandleRequiredFieldsChanged(object sender, EventArgs args)
        {
            StatusMessage = "";

            if (_serverTypeComboBox.SelectedIndex == (int)DirectoryServer.SERVER_TYPE.LDAP)
            {
                StatusMessage = _theConfigureDirectoryServerUserControl.DoValidate();
                if (String.IsNullOrEmpty(StatusMessage))
                {
                    _theAddButton.Enabled = true;
                }
                else
                {
                    _theAddButton.Enabled = false;
                }
            }
            else
            {
                StatusMessage = _theConfigureActiveDirectoryServerUserControl.DoValidate();
                if (String.IsNullOrEmpty(StatusMessage))
                {
                    _theAddButton.Enabled = true;
                }
                else
                {
                    _theAddButton.Enabled = false;
                }
            }
        }

        /// <summary>
        /// The event handler for cancel button clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCancelButton_Click(object sender, System.EventArgs e)
        {
            Close();
        }

        /// <summary>
        /// The event handler for Ok button clicked. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theOKButton_Click(object sender, System.EventArgs e)
        {
            if (!IsValidated())
            {
                return;
            }

            if (_theEditDialog)
            {
                // Alter 
                SetAttributes(_theDirectoryServer);
                try
                {
                    if (!_firstTime)
                    {
                        // To remember if more have been changed. 
                        _origUpdatedAttrVector |= _theDirectoryServer.UpdatedAttrVector;
                        _origCommonParameterUpdatedFlag |= _theDirectoryServer.CommonParametersUpdated;

                        // To reset the flag on the server since the 2nd Set might have reset all of the
                        // original flags. 
                        _theDirectoryServer.UpdatedAttrVector = _origUpdatedAttrVector;
                        _theDirectoryServer.CommonParametersUpdated = _origCommonParameterUpdatedFlag;
                    }

                    _theDirectoryServer.Alter();
                    _theDirectoryServer.AlterCommonConfigParameters();

                    DialogResult = DialogResult.Yes;
                }
                catch (Exception ex)
                {
                    String message = String.Format(Properties.Resources.ErrorOperationFailed,
                                                   Properties.Resources.EditServer,
                                                   ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle,
                                                  Properties.Resources.EditServer),
                                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                    _firstTime = false;
                }
            }
            else
            {
                // Create 
                _theDirectoryServer = new DirectoryServer(_theConnectionDefinition);
                SetAttributes(_theDirectoryServer);
                try
                {
                    _theDirectoryServer.AlterCommonConfigParameters();
                    _theDirectoryServer.Create();
                    DialogResult = DialogResult.Yes;
                }
                catch (Exception ex)
                {
                    String message = String.Format(Properties.Resources.ErrorOperationFailed,
                                                   Properties.Resources.CreateDirectoryServer,
                                                   ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle,
                                                  Properties.Resources.CreateDirectoryServer),
                                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private bool IsValidated()
        {
            if (_serverTypeComboBox.SelectedIndex == 0)
            {
                StatusMessage = _theConfigureDirectoryServerUserControl.DoValidate();
            }
            else
            {
                StatusMessage = _theConfigureActiveDirectoryServerUserControl.DoValidate();
            }

            if (String.IsNullOrEmpty(StatusMessage))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Set LDAP server attributes according to the user input from the dialog.
        /// </summary>
        /// <param name="server"></param>
        /// <param name="port"></param>
        private void SetAttributes(DirectoryServer server)
        {
            if (_serverTypeComboBox.SelectedIndex == 0)
            {
                _theConfigureDirectoryServerUserControl.SetServerAttributes(server);
            }
            else
            {
                _theConfigureActiveDirectoryServerUserControl.SetServerAttributes(server);
            }
        }

        /// <summary>
        /// Load LDAP server attributes from the model object.
        /// </summary>
        /// <param name="aDirectoryServer"></param>
        private void LoadAttributes(DirectoryServer aDirectoryServer, bool isReset)
        {
            if (null == aDirectoryServer)
            {
                // If this is a simple ADD, we'll just load the commom parameters.
                if (_commonParameters == null)
                {
                    try
                    {
                        _commonParameters = DirectoryServer.GetCommonConfigParameters(_theConnectionDefinition);
                        _errorEncountered = false;
                    }
                    catch (Exception ex)
                    {
                        _errorEncountered = true;
                        string message = String.Format(Properties.Resources.ErrorOperationFailed,
                               Properties.Resources.RetrieveCommonParameters,
                               ex.Message);
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                        message,
                                        String.Format(Properties.Resources.OperationErrorTitle,
                                                      (_theEditDialog ? Properties.Resources.EditServer :
                                                                        Properties.Resources.CreateDirectoryServer)),
                                        MessageBoxButtons.OK,
                                        MessageBoxIcon.Error);
                        if (isReset)
                        {
                            // Close the dialog if it is a reset.
                            DialogResult = DialogResult.Cancel;
                            Close();
                            return;
                        }
                    }
                }

                if (_serverTypeComboBox.SelectedIndex == 0)
                {
                    _theConfigureDirectoryServerUserControl.CommonConfigParameters = _commonParameters;
                }
                else
                {
                    _theConfigureActiveDirectoryServerUserControl.CommonConfigParameters = _commonParameters;
                }
            }
            else
            {
                // Set the server type first. After that the config user control should have been settled. 
                _serverTypeComboBox.SelectedIndex = (int)aDirectoryServer.ServerType;

                if (_serverTypeComboBox.SelectedIndex == 0)
                {
                    _theConfigureDirectoryServerUserControl.LoadAttributes(aDirectoryServer, _theEditDialog);
                }
                else
                {
                    _theConfigureActiveDirectoryServerUserControl.LoadAttributres(aDirectoryServer, _theEditDialog);
                }
            }
        }

        /// <summary>
        /// The event handler for reset button, which reset the dialog to the original values.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theResetButton_Click(object sender, System.EventArgs e)
        {
            Reset();
            if (_theDirectoryServer != null)
            {
                try
                {
                    _theDirectoryServer.Refresh();
                    _errorEncountered = false;
                }
                catch (Exception ex)
                {
                    _errorEncountered = true;
                    String message = String.Format(Properties.Resources.ErrorOperationFailed,
                                                   Properties.Resources.RefreshDirectoryServer,
                                                   ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle,
                                                  Properties.Resources.RefreshDirectoryServer),
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);

                    // Close the dialog if it is a reset.
                    DialogResult = DialogResult.Cancel;
                    Close();
                    return;
                }

                LoadAttributes(_theDirectoryServer, true);
            }
            else
            {
                LoadAttributes(_theDirectoryServer, true); // this will load the common parameters.
            }
        }

        #endregion Private methods

        /// <summary>
        /// Reset the GUI display.
        /// </summary>
        private void Reset()
        {
            if (_serverTypeComboBox.SelectedIndex == 0)
            {
                _theConfigureDirectoryServerUserControl.Reset();
            }
            else
            {
                _theConfigureActiveDirectoryServerUserControl.Reset();
            }
        }

        private void _serverTypeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            _directoryServerPanel.Controls.Clear();
            int dsControlHeight = 0;

            if (_commonParameters == null)
            {
                try
                {
                    _commonParameters = DirectoryServer.GetCommonConfigParameters(_theConnectionDefinition);
                    _errorEncountered = false;
                }
                catch (Exception ex)
                {
                    _errorEncountered = true;
                    string message = String.Format(Properties.Resources.ErrorOperationFailed,
                                                   Properties.Resources.RetrieveCommonParameters,
                                                   ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle,
                                                  (_theEditDialog ? Properties.Resources.EditServer :
                                                                    Properties.Resources.CreateDirectoryServer)),
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);

                    // Close the dialog only if it is initialized.
                    DialogResult = DialogResult.Cancel;
                    Close();
                    return;
                }
            }

            if (_serverTypeComboBox.SelectedIndex == 0)
            {
                _theConfigureDirectoryServerUserControl = new ConfigureDirectoryServerUserControl(_theEditDialog);
                _theConfigureDirectoryServerUserControl.OnRequiredFieldsChanged += HandleRequiredFieldsChanged;
                _theConfigureDirectoryServerUserControl.CommonConfigParameters = _commonParameters;
                dsControlHeight = _theConfigureDirectoryServerUserControl.Height + 2;
                _directoryServerPanel.Controls.Add(_theConfigureDirectoryServerUserControl);
            }
            else
            {
                _theConfigureActiveDirectoryServerUserControl = new ConfigureActiveDirectoryServerUserControl(_theEditDialog);
                _theConfigureActiveDirectoryServerUserControl.OnRequiredFieldsChanged += HandleRequiredFieldsChanged;
                _theConfigureActiveDirectoryServerUserControl.CommonConfigParameters = _commonParameters;
                dsControlHeight = _theConfigureActiveDirectoryServerUserControl.Height + 2;
                _directoryServerPanel.Controls.Add(_theConfigureActiveDirectoryServerUserControl);
            }

            Height = _intialDialogHeight - _initialServerControlHeight + dsControlHeight ;
            Update();
            _theAddButton.Enabled = IsValidated();
        }
    }
}
