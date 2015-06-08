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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.Connections.Controls
{
    /// <summary>
    /// The dialog for changing passwords
    /// </summary>
    public partial class ChangePasswordDialog : Form
    {
        #region Fields

        private ConnectionDefinition _theConnectionDefinition = null;
        private bool _showConfirmation = false;
        private TrafodionChangeTracker _theChangeTracker = null;

        #endregion Fields

        #region Properties 

        /// <summary>
        /// ConnectionDefn: the connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// StatusText: showing the error message
        /// </summary>
        public String StatusText
        {
            get { return _theStatusTextBox.Text; }
            set 
            { 
                _theStatusTextBox.Text = value;
                if (string.IsNullOrEmpty(_theStatusTextBox.Text))
                {
                    _theStatusTextBox.Visible = false;
                }
                else
                {
                    _theStatusTextBox.Visible = true;

                }
            }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The dialog for change user or role passwords.
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public ChangePasswordDialog(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
            _theChangePasswordUserControl.SystemName = ConnectionDefn.Name;
            _theChangePasswordUserControl.UserName = ConnectionDefn.UserName;
            StatusText = "";
            CenterToParent();

            //Add a tracker to track changes
            AddChangeTracker();
        }

        /// <summary>
        /// The dialog with the option of showing confirmation message if successful.
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aShowConfirmation"></param>
        public ChangePasswordDialog(ConnectionDefinition aConnectionDefinition, bool aShowConfirmation)
            : this(aConnectionDefinition)
        {
            _showConfirmation = aShowConfirmation;
            PerformValidation();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Reset the old and new passwords
        /// </summary>
        public void ResetDialog()
        {
            _theChangePasswordUserControl.OldPassword = "";
            _theChangePasswordUserControl.NewPassword = "";
            _theChangePasswordUserControl.ConfirmNewPassword = "";
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Monitoring changes.
        /// </summary>
        private void AddChangeTracker()
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
            _theChangeTracker = new TrafodionChangeTracker(_theChangePasswordUserControl);
            _theChangeTracker.OnChangeDetected += new TrafodionChangeTracker.ChangeDetected(ChangeTracker_OnChangeDetected);
            _theChangeTracker.EnableChangeEvents = true;
        }

        /// <summary>
        /// Event handler for change detected.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ChangeTracker_OnChangeDetected(object sender, System.EventArgs e)
        {
            PerformValidation();
        }

        /// <summary>
        /// Perform a proper validation, put out the status text and enable/disable the OK button.
        /// </summary>
        private void PerformValidation()
        {
            StatusText = _theChangePasswordUserControl.DoValidate();
            if (String.IsNullOrEmpty(StatusText))
            {
                _applyButton.Enabled = true;
            }
            else
            {
                _applyButton.Enabled = false;
            }
        }

        /// <summary>
        /// The event handler for cancel button. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _cancelButton_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        /// <summary>
        /// The event handler for apply button.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _applyButton_Click(object sender, EventArgs e)
        {
            string saveTheOldStatus = StatusText;
            StatusText = "";
            Connection conn = null;
            if (_theChangePasswordUserControl.ValidatePasswords())
            {
                // Invoking Change password SPJs.
                try
                {
                    conn = new Connection(ConnectionDefn, true); // this would not perform do test.
                    conn.ChangePassword(_theChangePasswordUserControl.UserName,
                                        _theChangePasswordUserControl.OldPassword,
                                        _theChangePasswordUserControl.NewPassword);

                    if (_showConfirmation)
                    {
                        String message = Properties.Resources.ChangePasswordSucceeded;
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                Properties.Resources.ChangePasswordSucceeded, 
                                Properties.Resources.ChangePasswordCompletedTitle, 
                                MessageBoxButtons.OK, 
                                MessageBoxIcon.Information);
                    }

                    DialogResult = DialogResult.OK;
                    Close();
                }
                catch (Exception ex)
                {
                    String message = String.Format(Properties.Resources.Error_ChangePasswordFailed, ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                            message, Properties.Resources.ChangePasswordFailedTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    ResetDialog();
                }
                finally
                {
                    // Close the connection explicit when it is done. 
                    conn.Close();
                }
            }
            else
            {
                if (String.IsNullOrEmpty(_theChangePasswordUserControl.OldPassword) ||
                    String.IsNullOrEmpty(_theChangePasswordUserControl.NewPassword))
                {
                    StatusText = Properties.Resources.Error_BlankPasswordNotAllowed;
                }
                else
                {
                    StatusText = Properties.Resources.Error_NewPasswordsDonotMatch;
                }
                // This will not close the dialog...
            };


        }

        #endregion Private methods
    }
}
