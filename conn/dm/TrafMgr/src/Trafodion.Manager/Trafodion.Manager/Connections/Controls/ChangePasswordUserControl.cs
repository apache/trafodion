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

namespace Trafodion.Manager.Connections.Controls
{
    /// <summary>
    /// User control for user to change own password
    /// </summary>
    public partial class ChangePasswordUserControl : UserControl
    {

        #region Fields

        string _systemName = null;
        string _userName = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// SystemName: The current system name.
        /// </summary>
        public String SystemName
        {
            get { return _theSystemTextBox.Text.Trim(); }
            set
            {
                _systemName = value;
                _theSystemTextBox.Text = value; 
            }
        }

        /// <summary>
        /// UserName: The user or role name for change password. 
        /// </summary>
        public String UserName
        {
            get { return _theUserNameTextBox.Text.Trim(); }
            set
            {
                _userName = value;
                _theUserNameTextBox.Text = value;
            }
        }

        /// <summary>
        /// OldPassword: the current password
        /// </summary>
        public String OldPassword
        {
            get { return _theOldPasswordTextBox.Text.Trim(); }
            set { _theOldPasswordTextBox.Text = value; }
        }

        /// <summary>
        /// NewPassword: the new password
        /// </summary>
        public String NewPassword
        {
            get { return _theNewPasswordTextBox.Text.Trim(); }
            set { _theNewPasswordTextBox.Text = value; }
        }

        /// <summary>
        /// ConfirmNewPassword: To confirm your new password
        /// </summary>
        public String ConfirmNewPassword
        {
            get { return _theConfirmNewPasswordTextBox.Text.Trim(); }
            set { _theConfirmNewPasswordTextBox.Text = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Default constructor
        /// </summary>
        public ChangePasswordUserControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Alternate constructor
        /// </summary>
        /// <param name="aUserName"></param>
        public ChangePasswordUserControl(string aUserName)
            : this()
        {
            UserName = aUserName;
            _theConfirmNewPasswordTextBox.MaxLength = ConnectionDefinition.PASSWORD_MAX_LENGTH;
            _theNewPasswordTextBox.MaxLength = ConnectionDefinition.PASSWORD_MAX_LENGTH;
            _theOldPasswordTextBox.MaxLength = ConnectionDefinition.PASSWORD_MAX_LENGTH;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To validate old and new passwords.  First, they all have to be none-empty strings. Secondly, 
        /// two new passwords have to match. 
        /// </summary>
        /// <returns></returns>
        public bool ValidatePasswords()
        {
            if (String.IsNullOrEmpty(OldPassword) ||
                String.IsNullOrEmpty(NewPassword) ||
                String.IsNullOrEmpty(ConfirmNewPassword))
            {
                return false;
            }
            else
            {
                if (NewPassword == ConfirmNewPassword)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            } 
        }

        /// <summary>
        /// Validate the passwords and return text for the status field.
        /// </summary>
        /// <returns></returns>
        public string DoValidate()
        {
            if (String.IsNullOrEmpty(OldPassword))
            {
                return "Please enter the old password";
            }

            if (String.IsNullOrEmpty(NewPassword))
            {
                return "Please enter the new password";
            }

            if (String.IsNullOrEmpty(ConfirmNewPassword))
            {
                return "Please confirm the new password";
            }

            if (NewPassword != ConfirmNewPassword)
            {
                return "New passwords do not match. Please re-confirm the new password";
            }

            return "";
        }

        #endregion Public methods
    }
}
