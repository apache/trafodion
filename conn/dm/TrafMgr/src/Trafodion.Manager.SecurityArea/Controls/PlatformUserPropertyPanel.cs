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
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class PlatformUserPropertyPanel : UserControl
    {
        User _theUser = null;
        User.EditMode _Mode = User.EditMode.Create;
        Policies _thePolicies = null;

        public PlatformUserPropertyPanel()
        {
            InitializeComponent();
            _thePasswordLabel.Text          = "Password";            
            _theConfirmPasswordLabel.Text   = "Confirm Password";

            _theDefaultSubVolumeLabel.Text  = "Default Subvolume";
            _theDefaultSubVolumeLabel.ShowRequired = false;

            _theInitialDirectoryLabel.Text  = "Initial Directory";
            _theInitialDirectoryLabel.ShowRequired = false;

            _theDefaultSecurityLabel.Text   = "Default Security";
            _theDefaultSecurityLabel.ShowRequired = false;


            _thePasswordText.MaxLength = 64;
            _theRetypePasswordText.MaxLength = 64;
            _theDefaultSubvolume.MaxLength = 32;
            _theInitialDirectory.MaxLength = 128;
            _theDefaultSecurity.MaxLength = 128;
            //setPasswordExpiryUi();
        }

        public User User
        {
            get
            {
                PopulateUiFromUser(_theUser);
                return _theUser;
            }
            set
            {
                _theUser = value;
                PopulateUiFromUser(_theUser);
            }
        }

        public User.EditMode Mode
        {
            get { return _Mode; }
            set
            {
                _Mode = value;
                _thePasswordLabel.ShowRequired = ((_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike));
                _theConfirmPasswordLabel.ShowRequired = ((_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike));
                _theDefaultSubVolumeLabel.ShowRequired = (_Mode == User.EditMode.Update);
                _theDefaultSecurityLabel.ShowRequired = (_Mode == User.EditMode.Update);
                _theInitialDirectoryLabel.ShowRequired = (_Mode == User.EditMode.Update);

            }
        }

        //Sets/Gets the value of the expiry days
        public int ExpiryDays
        {
            get
            {
                return _thePasswordExpirationPolicy.ExpiryDays;
            }
            set
            {
                _thePasswordExpirationPolicy.ExpiryDays = value;
            }
        }

        public bool NoExpiry
        {
            get
            {
                return _thePasswordExpirationPolicy.NoExpiry;
            }

            set
            {
                _thePasswordExpirationPolicy.NoExpiry = value;
            }

        }
        //Sets/Gets the value of the expiry date
        public string ExpiryDate
        {
            get
            {
                return _thePasswordExpirationPolicy.ExpiryDate;
            }

            set
            {
                _thePasswordExpirationPolicy.ExpiryDate = value;
            }
        }

        //The default policies
        public Policies Policies
        {
            get { return _thePolicies; }
            set { _thePolicies = value; }
        }

        public void ShowAdditionalProperties(bool show)
        {
            _theAdditionalParametersPanel.Visible = show;
        }

        public void SetDefaultPolicyValues(ConnectionDefinition aConnectionDefinition)
        {
            if (_Mode == User.EditMode.CreateLike)
            {
                //nothing to do here. The password expiry details are inherited from the source user.
            }
            else
            {
                if ((_Mode == User.EditMode.Create) || (PasswordNeverExpires(_theUser)))
                {
                    int defaultPasswordExpiryDays = 0;
                    string defaultPasswordExpiryDate = DateTime.Now.AddDays(5).ToShortDateString();
                    bool noExpiry = true;
                    GetDefaultPolicySettings(aConnectionDefinition);
                    if (_thePolicies != null)
                    {
                        defaultPasswordExpiryDays = _thePolicies.PwdDefaultExprDays;
                        defaultPasswordExpiryDate = _thePolicies.PwdDefaultExprDate;
                        noExpiry = ((defaultPasswordExpiryDays == 0) && ((defaultPasswordExpiryDate == null) || (defaultPasswordExpiryDate.Length == 0)));
                    }
                    _thePasswordExpirationPolicy.ExpiryDays = defaultPasswordExpiryDays;
                    _thePasswordExpirationPolicy.ExpiryDate = defaultPasswordExpiryDate;
                    _thePasswordExpirationPolicy.NoExpiry = noExpiry;
                }
            }
        }
        //private void setPasswordExpiryUi()
        //{
        //    if (_theNoExpirationRadio.Checked)
        //    {
        //        _theExpiryDate.Enabled = false;
        //        _theExpiryDays.Text = "";
        //        _theExpiryDays.Enabled = false;
        //    }
        //    else
        //    {
        //        _theExpiryDate.Enabled = true;
        //        _theExpiryDays.Text = "";
        //        _theExpiryDays.Enabled = true;
        //    }

        //}

        public void PopulateUiFromUser(User aUser)
        {
            if ((aUser.ExpirationDays > 0) || ((aUser.ExpirationDate != null) && (aUser.ExpirationDate.Trim().Length > 0)))
            {
                _thePasswordExpirationPolicy.ExpiryDays = aUser.ExpirationDays;
                _thePasswordExpirationPolicy.ExpiryDate = aUser.ExpirationDate.Trim();
                _thePasswordExpirationPolicy.NoExpiry = false;
            }
            else
            {
                _thePasswordExpirationPolicy.NoExpiry = true;
            }

            if (aUser.UserType == User.UserTypeEnum.PlatformUser)
            {
                _theAdditionalParametersPanel.Visible = true;
                _theDefaultSubvolume.Text = (aUser.DefaultSubvolume == null) ? "" : aUser.DefaultSubvolume.Trim().ToUpper();
                _theInitialDirectory.Text = (aUser.InitialDirectory == null) ? "" : aUser.InitialDirectory.Trim();
                _theDefaultSecurity.Text = (aUser.DefaultSecurity == null) ? "" : aUser.DefaultSecurity.Trim();
            }
            else
            {
                _theAdditionalParametersPanel.Visible = false;
                _theDefaultSubvolume.Text = "";
                _theInitialDirectory.Text = "";
                _theDefaultSecurity.Text  = "";
            }
        }

        public void PopulatePlatformUserProperties(User aUser)
        {
            aUser.Password = _thePasswordText.Text.Trim();
            int expirationDays = 0;
            if (!_thePasswordExpirationPolicy.NoExpiry)
            {
                aUser.ExpirationDays = _thePasswordExpirationPolicy.ExpiryDays;// (int.TryParse(_theExpiryDays.Text, out expirationDays)) ? expirationDays : 0;
                aUser.ExpirationDate = _thePasswordExpirationPolicy.ExpiryDate;// _theExpiryDate.Value.ToString("MM-dd-yyyy");
            }
            else
            {
                aUser.ExpirationDays = 0;
                aUser.ExpirationDate = "";
            }

            if (_theAdditionalParametersPanel.Visible)
            {
                aUser.DefaultSubvolume = _theDefaultSubvolume.Text.Trim().ToUpper();
                aUser.InitialDirectory = _theInitialDirectory.Text.Trim();
                aUser.DefaultSecurity = _theDefaultSecurity.Text.Trim().ToUpper();
            }
            else
            {
                aUser.DefaultSubvolume = "";
                aUser.InitialDirectory = "";
                aUser.DefaultSecurity  = "";
            }
        }

        public List<string> IsValid(User.EditMode aMode)
        {
            List<string> ret = new List<string>();


            if ((aMode == User.EditMode.Create) || (aMode == User.EditMode.CreateLike))
            {
                if (string.IsNullOrEmpty(_thePasswordText.Text))
                {
                    ret.Add("The password is required");
                }
                if (string.IsNullOrEmpty(_theRetypePasswordText.Text))
                {
                    ret.Add("Please retype the password");
                }
                if (! _thePasswordText.Text.Equals(_theRetypePasswordText.Text))
                {
                    ret.Add("The entered password and the retyped password doesn't match");
                }
                //if ((!string.IsNullOrEmpty(_thePasswordText.Text))
                //    && (!string.IsNullOrEmpty(_theRetypePasswordText.Text))
                //    && (_thePasswordText.Text.Equals(_theRetypePasswordText.Text)))
                //{
                //    ret.Add("The password ");
                //}
            }
            else if (aMode == User.EditMode.Update)
            {
                //return (_thePasswordText.Text.Equals(_theRetypePasswordText.Text));
                if (!_thePasswordText.Text.Equals(_theRetypePasswordText.Text))
                {
                    ret.Add("The entered password and the retyped password doesn't match");
                }

                if ((_theUser != null) && (_theUser.UserType == User.UserTypeEnum.PlatformUser))
                {
                    if (string.IsNullOrEmpty(this._theDefaultSubvolume.Text))
                    {
                        ret.Add("The default subvolume is required");
                    }
                    if (string.IsNullOrEmpty(this._theInitialDirectory.Text))
                    {
                        ret.Add("The initial directory is required");
                    }
                    if (string.IsNullOrEmpty(this._theDefaultSecurity.Text))
                    {
                        ret.Add("The default security is required");
                    }
                }
            }
            return ret;
        }

        private void GetDefaultPolicySettings(ConnectionDefinition aConnectionDefinition)
        {
            if (_thePolicies == null)
            {
                SystemSecurity systemSecurity = SystemSecurity.FindSystemModel(aConnectionDefinition);
                _thePolicies = systemSecurity.Policies;
                TrafodionProgressArgs args = new TrafodionProgressArgs("Looking up default policy settings...", _thePolicies, "GetAttributes", new Object[] { });
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();
                if (progressDialog.Error != null)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error obtaining default policy settings",
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    _thePolicies = null;
                }
            }
        }
        private bool PasswordNeverExpires(User aUser)
        {
            if (aUser == null)
            {
                return true;
            }
            else
            {
                return ((aUser.ExpirationDays == 0) && ((aUser.ExpirationDate == null) || (aUser.ExpirationDate.Trim().Length == 0)));
            }
        }

    }
}
