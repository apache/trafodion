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
using Trafodion.Manager.SecurityArea.Model;
using System.Collections;

namespace Trafodion.Manager.SecurityArea.Controls
{
    /// <summary>
    /// User control for configuring a LDAP directory server entry.
    /// </summary>
    public partial class ConfigureDirectoryServerUserControl : UserControl
    {
        #region Fields

        /// <summary>
        /// Delegate for required fields changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        public delegate void RequiredFieldsChanged(object sender, EventArgs args);

        /// <summary>
        /// Event for required fields changed.
        /// </summary>
        public event RequiredFieldsChanged OnRequiredFieldsChanged;

        private DirectoryServerParametersUserControl _parametersUserControl = null;
        private DirectoryServerParametersUserControl _commonParametersUserControl = null;
        private string _theDomainName = DirectoryServer.DEFAULT_LDAP_DOMAIN_NAME;
        private bool _theEditDialog = false;
        private string _theCACertCache = "";
        private string ANONYMOUS_SEARCH = "ANONYMOUS";
        private string USERNAME_SEARCH = "User Name + Password";

        #endregion Fields

        #region Properties

        /// <summary>
        /// IsEditDialog: is this an edit dialog?
        /// </summary>
        public bool IsEditDialog
        {
            get { return _theEditDialog; }
        }

        /// <summary>
        /// DomainName: domain name of the directory server.
        /// </summary>
        public String DomainName
        {
            get { return _theDomainName; }
        }

        /// <summary>
        /// UsagePriority: the usage priority of the directory server.
        /// </summary>
        public String UsagePriority
        {
            get { return _theUsagePriorityTextBox.Text.Trim(); }
            set 
            {
                if (_theUsagePriorityTextBox.Enabled)
                {
                    _theUsagePriorityTextBox.Text = value;
                }
            }
        }

        /// <summary>
        /// UsagePriorityReadOnly: whether the usage priority is readonly?
        /// </summary>
        public bool UsagePriorityReadOnly
        {
            get { return !_theUsagePriorityTextBox.Enabled; }
            set { _theUsagePriorityTextBox.Enabled = !value; }
        }

        /// <summary>
        ///  HostName: the host name of the directory server.
        /// </summary>
        public String HostName
        {
            get { return _theHostNameTextBox.Text.Trim(); }
            set { _theHostNameTextBox.Text = value; }
        }

        /// <summary>
        /// PortNumber: the port number of the directory server.
        /// </summary>
        public String PortNumber
        {
            get { return _thePortNumberTextBox.Text.Trim(); }
            set { _thePortNumberTextBox.Text = value; }
        }

        /// <summary>
        /// SearchUserDN: the serach user DN used for the directory server. 
        /// </summary>
        public String SearchUserDN
        {
            get { return _theSearchUserDNTextBox.Text.Trim(); }
            set { _theSearchUserDNTextBox.Text = value; }
        }

        /// <summary>
        /// SearchUserPassword: the search user password for the directory server. 
        /// </summary>
        public String SearchUserPassword
        {
            get { return _theSearchUserPasswordTextBox.Text.Trim(); }
            set { _theSearchUserPasswordTextBox.Text = value; }
        }

        /// <summary>
        /// SearchUserConfirmPassword: the search user confirm password for the directory server. 
        /// Note: This is just to verify the new password entered. 
        /// </summary>
        public String SearchUserConfirmPassword
        {
            get { return _theSearchUserConfirmPasswordTextBox.Text.Trim(); }
            set { _theSearchUserConfirmPasswordTextBox.Text = value; }
        }

        /// <summary>
        /// LDAPVersion: the LDAP server version of the directory server. 
        /// </summary>
        public int LDAPVersion
        {
            get
            {
                if (_theVersion3RadioButton.Checked)
                {
                    return 3;
                }
                else
                {
                    return 2;
                }
            }
            set
            {
                if (value == 2)
                {
                    _theVersion2RadioButton.Checked = true;
                }
                else
                {
                    _theVersion3RadioButton.Checked = true;
                }
            }
        }

        /// <summary>
        /// Encryption: the SSL encryption argorithm used by the directory server. 
        /// </summary>
        public short Encryption
        {
            get { return (short)(_theSSLOptionComboBox.SelectedIndex); }
            set { _theSSLOptionComboBox.SelectedIndex = value; }
        }

        /// <summary>
        /// CACert: the certificate of the directory server. 
        /// </summary>
        public String CACert
        {
            get { return _theCertificateMultilineTextUserControl.Content; }
            set { _theCACertCache = _theCertificateMultilineTextUserControl.Content = value; }
        }

        /// <summary>
        /// ConfigParameters: the configuration parameters for the directory server. 
        /// </summary>
        public ArrayList ConfigParameters
        {
            get { return _parametersUserControl.Parameters; }
            set 
            { 
                _parametersUserControl.Parameters = value;
                FireRequiredFieldsChanged(new EventArgs());
            }
        }

        /// <summary>
        /// CommonConfigParameters: the common parameters of the active directory.
        /// </summary>
        public ArrayList CommonConfigParameters
        {
            get { return _commonParametersUserControl.Parameters; }
            set
            {
                _commonParametersUserControl.Parameters = value;
            }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The default constructor
        /// </summary>
        public ConfigureDirectoryServerUserControl(bool anEditDialog)
        {
            _theEditDialog = anEditDialog;
            InitializeComponent();

            _parametersUserControl = new DirectoryServerParametersUserControl(DirectoryServer.CONFIG_PARAMETERS_TYPE.LDAP_CONFIG);
            _parametersUserControl.Dock = DockStyle.Fill;
            _theConfigParametersPanel.Controls.Add(_parametersUserControl);
            //_parametersUserControl.OnRequiredFieldsChanged += RequiredFields_TextChanged;

            this._searchTypeGuiComboBox.Items.AddRange(new object[] {
                    ANONYMOUS_SEARCH,USERNAME_SEARCH});
            _searchTypeGuiComboBox.SelectedItem = USERNAME_SEARCH;
            this._searchTypeGuiComboBox.SelectedIndexChanged += new System.EventHandler(this._searchTypeGuiComboBox_SelectedIndexChanged);

            // Create both user controls for the common parameters
            // But, let the select of domain type to decide which one to put into the GUI 
            _commonParametersUserControl =
                new DirectoryServerParametersUserControl(DirectoryServer.CONFIG_PARAMETERS_TYPE.LDAP_COMMON_CONFIG);
            _commonParametersUserControl.Dock = DockStyle.Fill;
            _theCommonParametersPanel.Controls.Add(_commonParametersUserControl);

            _theSSLOptionComboBox.Items.Clear();
            foreach (string option in DirectoryServer.ENCRYPTION_SUPPORT_LABEL)
            {
                _theSSLOptionComboBox.Items.Add(option);
            }

            // Be sure to use the property to do it since there is a index offset to deal with.
            Encryption = DirectoryServer.DEFAULT_ENCRYPTION;

            _theCertificateMultilineTextUserControl.ImportButton.Click += CertificateFileImportButton_Click;
            _theCertificateMultilineTextUserControl.TextBox.TextChanged += RequiredFields_TextChanged;
            _theCertificateMultilineTextUserControl.ImportButtonToolTipText = Properties.Resources.DirectoryServerCertificateToolTipText;
            this.Disposed += IAM_Disposed;

            if (IsEditDialog)
            {
                RemoveRequiredLabel();
            }
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To reset the user control to its default setting. 
        /// </summary>
        public void Reset()
        {
            _searchTypeGuiComboBox.SelectedItem = USERNAME_SEARCH;
            UsagePriorityReadOnly = false;
            UsagePriority = "";
            HostName = "";
            PortNumber = "";
            SearchUserDN = "";
            _theSearchUserPasswordTextBox.Text = "";
            _theSearchUserConfirmPasswordTextBox.Text = "";
            ConfigParameters = null;
            CommonConfigParameters = null;
            CACert = "";
            LDAPVersion = DirectoryServer.DEFAULT_SERVER_VERSION;
            Encryption = DirectoryServer.DEFAULT_ENCRYPTION;
        }

        /// <summary>
        /// Load attributes from a directory server model.
        /// </summary>
        /// <param name="aDirectoryServer"></param>
        /// <param name="editDialog"></param>
        public void LoadAttributes(DirectoryServer aDirectoryServer, bool editDialog)
        {
            if (editDialog)
            {
                // show passwords only for editing
                if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERPASSWORD))
                {
                    SearchUserPassword = aDirectoryServer.SearchUserPassword;
                    SearchUserConfirmPassword = aDirectoryServer.SearchUserPassword;
                }

                UsagePriority = aDirectoryServer.UsagePriority.ToString();
                UsagePriorityReadOnly = true;
            }

            if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_HOST))
            {
                HostName = aDirectoryServer.HostName;
            }

            if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_PORT_NUMBER))
            {
                PortNumber = aDirectoryServer.PortNumber.ToString();
            }

            if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_VERSION))
            {
                LDAPVersion = aDirectoryServer.Version;
            }

            if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_ENCRYPTION))
            {
                Encryption = aDirectoryServer.Encryption;
            }

            if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERDN))
            {
                SearchUserDN = aDirectoryServer.SearchUserDN;
            }

            if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_CACERT))
            {
                CACert = aDirectoryServer.CACert;
            }

            if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_CONFIGTEXT))
            {
                ConfigParameters = aDirectoryServer.ConfigParameters;
            }

            if (ANONYMOUS_SEARCH.Equals(aDirectoryServer.SearchUserDN, StringComparison.InvariantCultureIgnoreCase))
            {
                SearchUserDN = "";
                _searchTypeGuiComboBox.SelectedItem = ANONYMOUS_SEARCH;
            }
            else
                _searchTypeGuiComboBox.SelectedItem = USERNAME_SEARCH;

            CommonConfigParameters = aDirectoryServer.CommonConfigParameters;
        }

        /// <summary>
        /// Validate the input and return the first error encountered.
        /// </summary>
        /// <returns></returns>
        public String DoValidate()
        {
            if (IsEditDialog)
            {
                // Since NCI does not require all fields, do not enforce required fields for Editing a possible entry 
                // created by NCI.
                if (!String.IsNullOrEmpty(PortNumber))
                {
                    try
                    {
                        int port = int.Parse(PortNumber);
                        if (1 > port || port > 65535)
                        {
                            return Properties.Resources.ErrorPortNumberRange;
                        }
                    }
                    catch (Exception)
                    {
                        return String.Format(Properties.Resources.ErrorNumericValueOnly, Properties.Resources.DSPropertyHostPortNumber);
                    }
                }

                //If not anonymous, then check for not null values in user info
                if (_searchTypeGuiComboBox.SelectedItem == null || _searchTypeGuiComboBox.SelectedItem.Equals(USERNAME_SEARCH))
                {
                    // If search user DN is present, insist to have user password
                    if (!String.IsNullOrEmpty(SearchUserDN))
                    {
                        if (String.IsNullOrEmpty(SearchUserPassword))
                        {
                            return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyPassword);
                        }
                    }

                    // If user password is present, confirm the password.
                    if (!String.IsNullOrEmpty(SearchUserPassword))
                    {
                        if (String.IsNullOrEmpty(SearchUserConfirmPassword))
                        {
                            return Properties.Resources.ErrorConfirmSearchUserPassword;
                        }

                        if (!SearchUserPassword.Equals(SearchUserConfirmPassword))
                        {
                            return Properties.Resources.ErrorPasswordsDonotMatch;
                        }
                    }
                }

                if ((Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.SSL ||
                     Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.TLS) &&
                    String.IsNullOrEmpty(CACert))
                {
                    return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyCertificate);
                }

                if (!String.IsNullOrEmpty(CACert) && CACert.Length > 4096)
                {
                    return Properties.Resources.ErrorCertificateRange;
                }

                return "";
            }

            // This is a ADD command.
            if (String.IsNullOrEmpty(HostName))
            {
                return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyHostName);
            }

            if (String.IsNullOrEmpty(PortNumber))
            {
                return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyHostPortNumber);
            }

            try
            {
                int port = int.Parse(PortNumber);
                if (1 > port || port > 65535)
                {
                    return Properties.Resources.ErrorPortNumberRange;
                }
            }
            catch (Exception)
            {
                return String.Format(Properties.Resources.ErrorNumericValueOnly, Properties.Resources.DSPropertyHostPortNumber);
            }

            if (String.IsNullOrEmpty(UsagePriority))
            {
                return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyUsagePriority);
            }

            try
            {
                int priority = int.Parse(UsagePriority);
                if (0 > priority || priority > 32767)
                {
                    return Properties.Resources.ErrorUsagePriorityRange;
                }
            }
            catch (Exception ex)
            {
                //return "Usage priority must be a numeric value";
                return String.Format(Properties.Resources.ErrorNumericValueOnly, Properties.Resources.DSPropertyUsagePriority);
            }

            //If not anonymous, then check for not null values in user info
            if (_searchTypeGuiComboBox.SelectedItem == null || _searchTypeGuiComboBox.SelectedItem.Equals(USERNAME_SEARCH))
            {
                if (String.IsNullOrEmpty(SearchUserDN))
                {
                    return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertySearchUserDN);
                }

                if (String.IsNullOrEmpty(SearchUserPassword))
                {
                    return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyPassword);
                }

                if (String.IsNullOrEmpty(SearchUserConfirmPassword))
                {
                    return Properties.Resources.ErrorConfirmSearchUserPassword;
                }

                if (!SearchUserPassword.Equals(SearchUserConfirmPassword))
                {
                    return Properties.Resources.ErrorPasswordsDonotMatch;
                }
            }

            if ((Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.SSL ||
                 Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.TLS) &&
                String.IsNullOrEmpty(CACert))
            {
                return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyCertificate);
            }

            if (!String.IsNullOrEmpty(CACert) && CACert.Length > 4096)
            {
                return Properties.Resources.ErrorCertificateRange;
            }

            if ((Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.SSL ||
                 Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.TLS) &&
                String.IsNullOrEmpty(CACert))
            {
                return "Certificate is required";
            }

            return "";
        }

        /// <summary>
        /// Set server attributes according to UI
        /// Note: It is assumed that the UI has been validated. 
        /// </summary>
        /// <param name="server"></param>
        public void SetServerAttributes(DirectoryServer server)
        {
            if (!IsEditDialog)
            {
                server.ServerType = DirectoryServer.SERVER_TYPE.LDAP;
                server.DomainName = DirectoryServer.DEFAULT_LDAP_DOMAIN_NAME;
                server.UsagePriority = short.Parse(UsagePriority);
            }

            server.HostName = HostName;
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_HOST);


            server.PortNumber = int.Parse(PortNumber);
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_PORT_NUMBER);
            if (_searchTypeGuiComboBox.SelectedItem.Equals(ANONYMOUS_SEARCH))
            {
                server.SearchUserDN = ANONYMOUS_SEARCH;
            }
            else
            {
                server.SearchUserDN = SearchUserDN;
            }
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERDN);

            server.SearchUserPassword = SearchUserPassword;
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERPASSWORD);

            server.Version = LDAPVersion;
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_VERSION);

            server.Encryption = Encryption;
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_ENCRYPTION);

            server.ConfigParameters = ConfigParameters;
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_CONFIGTEXT);
 
            if (Encryption != (short)DirectoryServer.ENCRYPTION_SUPPORT.NONE && 
                !String.IsNullOrEmpty(CACert))
            {
                server.CACert = CACert;
                server.SetPresentVector(DirectoryServer.ATTR_VECTOR_CACERT);
            }
            else
            {
                server.CACert = "";
            }

            server.CommonConfigParameters = CommonConfigParameters;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Event handler if I am disposed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void IAM_Disposed(object sender, EventArgs e)
        {
            _theCertificateMultilineTextUserControl.ImportButton.Click -= CertificateFileImportButton_Click;
        }

        /// <summary>
        /// Certificate file import button event handler.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CertificateFileImportButton_Click(object sender, EventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            DialogResult result = dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                try
                {
                    string s = System.IO.File.ReadAllText(dialog.FileName);
                    CACert = s.Trim();
                }
                catch (Exception ex)
                {
                    String message = String.Format(Properties.Resources.ErrorOperationFailed,
                                                   Properties.Resources.ImportCertificate,
                                                   ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle,
                                                  Properties.Resources.ImportCertificate),
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                }
            }
        }

        /// <summary>
        /// Event handler for SL option combox box changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theSSLOptionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.SSL ||
                Encryption == (short)DirectoryServer.ENCRYPTION_SUPPORT.TLS)
            {
                _theCertificateMultilineTextUserControl.Enabled = true;
                _theCertificateMultilineTextUserControl.Content = _theCACertCache;
            }
            else
            {
                _theCertificateMultilineTextUserControl.Enabled = false;

                // Also, clear the text box.
                _theCertificateMultilineTextUserControl.Content = "";
            }
            FireRequiredFieldsChanged(new EventArgs());
        }

        /// <summary>
        /// Event handler for required fields changed. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void RequiredFields_TextChanged(object sender, EventArgs e)
        {
            FireRequiredFieldsChanged(new EventArgs());
        }

        /// <summary>
        /// Fire events if required field changed.
        /// </summary>
        /// <param name="e"></param>
        private void FireRequiredFieldsChanged(EventArgs e)
        {
            if (OnRequiredFieldsChanged != null)
            {
                OnRequiredFieldsChanged(this, e);
            }
        }

        private void RemoveRequiredLabel()
        {
            _theRequiredLabel2.Text = " ";
            _theRequiredLabel3.Text = " ";
            _theRequiredLabelDN.Text = " ";
            _theRequiredLabelPWD.Text = " ";
            _theRequiredLabelCPW.Text = " ";
            _theRequiredLabel7.Text = " ";
        }

        /// <summary>
        /// To track a numeric textbox key pressed and make sure only numeric digits are accepted. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void NumericTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (Char.IsDigit(e.KeyChar))
            {
                // Digits are OK
            }

            else if (e.KeyChar == '\b')
            {
                // Backspace key is OK
            }
            else if ((ModifierKeys & (Keys.Control | Keys.Alt)) != 0)
            {
                // Let the edit control handle control and alt key combinations
            }
            else
            {
                // Consume this invalid key and beep
                e.Handled = true;
                // MessageBeep();
            }
        }

        #endregion Private methods


        private void _searchTypeGuiComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {

            if (_searchTypeGuiComboBox.SelectedItem.Equals(ANONYMOUS_SEARCH))
            {
                _searchDNLabel.Enabled = _passwordLabel.Enabled = _confrmPasswordLabel.Enabled = false;
                _theSearchUserDNTextBox.Enabled = _theSearchUserPasswordTextBox.Enabled = _theSearchUserConfirmPasswordTextBox.Enabled = false;
                _theRequiredLabelDN.Text = _theRequiredLabelCPW.Text = _theRequiredLabelPWD.Text = " ";
                _theSearchUserDNTextBox.Text = _theSearchUserPasswordTextBox.Text = _theSearchUserConfirmPasswordTextBox.Text = "";
            }
            else
            {
                _theSearchUserDNTextBox.Text = "";
                _searchDNLabel.Enabled = _passwordLabel.Enabled = _confrmPasswordLabel.Enabled = true;
                _theSearchUserDNTextBox.Enabled = _theSearchUserPasswordTextBox.Enabled = _theSearchUserConfirmPasswordTextBox.Enabled = true;
                _theRequiredLabelDN.Text = _theRequiredLabelCPW.Text = _theRequiredLabelPWD.Text = "*";
            }
            FireRequiredFieldsChanged(new EventArgs());
        }
    }
}
