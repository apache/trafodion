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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework.Controls;
using System.Collections;

namespace Trafodion.Manager.SecurityArea.Controls
{
    /// <summary>
    /// User control for configuring a active directory server entry.
    /// </summary>
    public partial class ConfigureActiveDirectoryServerUserControl : UserControl
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

        // The _commonParametersGrid is used only for Domain Controller
        private TrafodionIGrid _commonParametersGrid = new TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0ATQB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAIABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
        private DirectoryServerParametersUserControl _parametersUserControl = null;

        // The _commonParametersUserControl is used only for Golbal Catalog
        private DirectoryServerParametersUserControl _commonParametersUserControl = null;
        private bool _theEditDialog = false;
        private string _theCACertCache = "";

        #endregion Fields

        #region Properties

        /// <summary>
        /// IsEditDialog: if this is a edit mode?
        /// </summary>
        public bool IsEditDialog
        {
            get { return _theEditDialog; }
        }


        /// <summary>
        /// DomainType: the domain type of the active directory
        /// </summary>
        public int DomainType
        {
            get { return _theDomainTypeComboBox.SelectedIndex; }
            set { _theDomainTypeComboBox.SelectedIndex = value; }
        }

        /// <summary>
        /// DomainName: the domain name of the active directory
        /// </summary>
        public String DomainName
        {
            get { return _theDomainNameTextBox.Text.Trim(); }
            set { _theDomainNameTextBox.Text = value; }
        }

        /// <summary>
        /// DomainNameReadOnly: if the domain name field is read only.
        /// </summary>
        public bool DomainNameReadOnly
        {
            get { return !_theDomainNameTextBox.Enabled; }
            set { _theDomainNameTextBox.Enabled = !value; }
        }

        /// <summary>
        /// UsagePriority: the usage priority of the active directory
        /// </summary>
        public String UsagePriority
        {
            get { return _theUsagePriorityTextBox.Text.Trim(); }
            set { _theUsagePriorityTextBox.Text = value; }
        }

        /// <summary>
        /// UsagePriorityReadOnly: if the usage priority field is read only.
        /// </summary>
        public bool UsagePriorityReadOnly
        {
            get { return !_theUsagePriorityTextBox.Enabled; }
            set { _theUsagePriorityTextBox.Enabled = !value; }
        }

        /// <summary>
        ///  HostName: the host name of the active directory server.
        /// </summary>
        public String HostName
        {
            get { return _theHostNameTextBox.Text.Trim(); }
            set { _theHostNameTextBox.Text = value; }
        }

        /// <summary>
        /// PortNumber: the port number of the active directory server.
        /// </summary>
        public String PortNumber
        {
            get { return _thePortNumberTextBox.Text.Trim(); }
            set { _thePortNumberTextBox.Text = value; }
        }

        /// <summary>
        /// SearchUserDN: the serach user DN used for the active directory server. 
        /// </summary>
        public String SearchUserDN
        {
            get { return _theSearchUserDNTextBox.Text.Trim(); }
            set { _theSearchUserDNTextBox.Text = value; }
        }

        /// <summary>
        /// SearchUserPassword: the search user password for the active directory server. 
        /// </summary>
        public String SearchUserPassword
        {
            get { return _theSearchUserPasswordTextBox.Text.Trim(); }
            set { _theSearchUserPasswordTextBox.Text = value; }
        }

        /// <summary>
        /// SearchUserConfirmPassword: the search user confirm password for the active directory server. 
        /// Note: This is just to verify the new password entered. 
        /// </summary>
        public String SearchUserConfirmPassword
        {
            get { return _theSearchUserConfirmPasswordTextBox.Text.Trim(); }
            set { _theSearchUserConfirmPasswordTextBox.Text = value; }
        }

        /// <summary>
        /// LDAPVersion: the LDAP server version of the active directory server. 
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
        /// Encryption: the SSL encryption argorithm used by the active directory server. 
        /// </summary>
        public short Encryption
        {
            get { return (short)(_theSSLOptionComboBox.SelectedIndex); }
            set { _theSSLOptionComboBox.SelectedIndex = value; }
        }

        /// <summary>
        /// CACert: the certificate of the active directory server. 
        /// </summary>
        public String CACert
        {
            get { return _theCertificateMultilineTextUserControl.Content; }
            set { _theCACertCache = _theCertificateMultilineTextUserControl.Content = value; }
        }

        /// <summary>
        /// ConfigParameters: the configuration parameters for the active directory server. 
        /// </summary>
        public ArrayList ConfigParameters
        {
            get { return _parametersUserControl.Parameters; }
            set { _parametersUserControl.Parameters = value; }
        }

        /// <summary>
        /// CommonConfigParameters: the common parameters of the active directory.
        /// </summary>
        public ArrayList CommonConfigParameters
        {
            get { return _commonParametersUserControl.Parameters; }
            set 
            { 
                // Set the new values to both controls.
                _commonParametersUserControl.Parameters = value;
                ResetCommonParametersGrid(value);
            }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The default constructor
        /// </summary>
        public ConfigureActiveDirectoryServerUserControl(bool anEditDialog)
        {
            _theEditDialog = anEditDialog;
            InitializeComponent();
            _parametersUserControl =  
                new DirectoryServerParametersUserControl(DirectoryServer.CONFIG_PARAMETERS_TYPE.AD_CONFIG);
            _parametersUserControl.Dock = DockStyle.Fill;
            _theConfigParametersPanel.Controls.Add(_parametersUserControl);

            // Create both user controls for the common parameters
            // But, let the select of domain type to decide which one to put into the GUI 
            _commonParametersUserControl =
                new DirectoryServerParametersUserControl(DirectoryServer.CONFIG_PARAMETERS_TYPE.AD_COMMON_CONFIG);
            _commonParametersUserControl.Dock = DockStyle.Fill;
            _commonParametersGrid.Dock = DockStyle.Fill;
            _commonParametersGrid.DoubleClickHandler = CommonParametersGridDoubleClickHandler;

            // Populate the encryption options
            _theSSLOptionComboBox.Items.Clear();
            foreach (string option in DirectoryServer.ENCRYPTION_SUPPORT_LABEL)
            {
                _theSSLOptionComboBox.Items.Add(option);
            }

            // Populate the domain type options
            _theDomainTypeComboBox.Items.Clear();
            foreach (string option in DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE_LABEL)
            {
                _theDomainTypeComboBox.Items.Add(option);
            }

            // Be sure to use the property to do it since there is a index offset to deal with.
            Encryption = DirectoryServer.DEFAULT_ENCRYPTION;

            _theCertificateMultilineTextUserControl.ImportButton.Click += CertificateFileImportButton_Click;
            _theCertificateMultilineTextUserControl.TextBox.TextChanged += RequiredFields_TextChanged;
            _theCertificateMultilineTextUserControl.ImportButtonToolTipText = Properties.Resources.DirectoryServerCertificateToolTipText;

            // Select the GC as the default
            _theDomainTypeComboBox.SelectedIndex = (int)DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG;

            this.Disposed += IAM_Disposed;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To reset the user control to its default setting. 
        /// </summary>
        public void Reset()
        {
            // not to reset the domain type.
            //_theDomainTypeComboBox.SelectedIndex = (int)DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG;
            if (_theDomainTypeComboBox.SelectedIndex != (int)DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG)
            {
                DomainName = "";
            }
            UsagePriority = "";
            UsagePriorityReadOnly = false;
            HostName = "";
            PortNumber = "";
            SearchUserDN = "";
            _theSearchUserPasswordTextBox.Text = "";
            _theSearchUserConfirmPasswordTextBox.Text = "";
            //ResetCommonParametersGrid(null);
            CommonConfigParameters = null;
            ConfigParameters = null;
            CACert = "";
            LDAPVersion = DirectoryServer.DEFAULT_SERVER_VERSION;
            Encryption = DirectoryServer.DEFAULT_ENCRYPTION;
        }

        /// <summary>
        /// Load attributes from a directory server model.
        /// </summary>
        /// <param name="aDirectoryServer"></param>
        /// <param name="aEditDialog"></param>
        public void LoadAttributres(DirectoryServer aDirectoryServer, bool isEditDialog)
        {
            if (isEditDialog)
            {
                // show passwords only for editing
                if (aDirectoryServer.CheckPresentVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERPASSWORD))
                {
                    SearchUserPassword = aDirectoryServer.SearchUserPassword;
                    SearchUserConfirmPassword = aDirectoryServer.SearchUserPassword;
                }

                UsagePriority = aDirectoryServer.UsagePriority.ToString();
                UsagePriorityReadOnly = true;
                DomainName = aDirectoryServer.DomainName;
                DomainNameReadOnly = true;
            }
            else
            {
                if (aDirectoryServer.DomainType == DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG)
                {
                    DomainName = DirectoryServer.DEFAULT_ACTIVE_DIRECTORY_GLOBAL_CATALOG_DOMAIN_NAME;
                }
                else
                {
                    DomainName = "";
                }
            }

            _theDomainTypeComboBox.SelectedIndex = (int)aDirectoryServer.DomainType;

            if (isEditDialog)
            {
                // These fields are also not allowed to be changed.
                _theDomainTypeComboBox.Enabled = false;
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

            CommonConfigParameters = aDirectoryServer.CommonConfigParameters;
        }

        /// <summary>
        /// Validate the input and return the first error encountered.
        /// </summary>
        /// <returns></returns>
        public String DoValidate()
        {
            if (DomainType == (int)DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.DOMAIN_CONTROLLER)
            {
                if (String.IsNullOrEmpty(DomainName))
                {
                    return String.Format(Properties.Resources.ErrorMissingRequiredField, Properties.Resources.DSPropertyDomainName);
                }
            }

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
            catch (Exception ex)
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
                return String.Format(Properties.Resources.ErrorNumericValueOnly, Properties.Resources.DSPropertyUsagePriority);
            }

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

        /// <summary>
        /// Set server attributes according to UI
        /// Note: It is assumed that the UI has been validated. 
        /// </summary>
        /// <param name="server"></param>
        public void SetServerAttributes(DirectoryServer server)
        {
            if (!IsEditDialog)
            {
                server.ServerType = DirectoryServer.SERVER_TYPE.ACTIVE_DIRECTORY;
                server.DomainType = (DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE)_theDomainTypeComboBox.SelectedIndex; 

                if (_theDomainTypeComboBox.SelectedIndex == (int)DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG)
                {
                    server.DomainName = DirectoryServer.DEFAULT_ACTIVE_DIRECTORY_GLOBAL_CATALOG_DOMAIN_NAME;
                }
                else
                {
                    server.DomainName = DomainName;
                }

                server.UsagePriority = short.Parse(UsagePriority);
            }

            server.HostName = HostName;
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_HOST);


            server.PortNumber = int.Parse(PortNumber);
            server.SetPresentVector(DirectoryServer.ATTR_VECTOR_PORT_NUMBER);

            server.SearchUserDN = SearchUserDN;
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

            if (_theDomainTypeComboBox.SelectedIndex == (int)DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG)
            {
                server.CommonConfigParameters = CommonConfigParameters;
            }
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

        /// <summary>
        /// Event handler for common parameter grid.
        /// </summary>
        /// <param name="rowIndex"></param>
        private void CommonParametersGridDoubleClickHandler(int rowIndex)
        {
            // do nothing here.
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

        private void ResetCommonParametersGrid(ArrayList paramList)
        {
            _commonParametersGrid.BeginUpdate();
            _commonParametersGrid.Rows.Clear();
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add("Name");
            dataTable.Columns.Add("Value");
            if (paramList != null)
            {
                foreach (string[] pair in paramList.ToArray())
                {
                    dataTable.Rows.Add(new object[] { pair[0], pair[1] });
                }
            }
            _commonParametersGrid.FillWithData(dataTable);
            _commonParametersGrid.AutoResizeCols = true;
            _commonParametersGrid.EndUpdate();
        }


        private void _theDomainTypeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (_theDomainTypeComboBox.SelectedIndex == (int)DirectoryServer.ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG)
            {
                _domainNameLabel.Enabled = _theDomainNameRequiredLabel.Enabled = _theDomainNameTextBox.Enabled = false;
                _theCommonParametersPanel.Controls.Clear();
                _theCommonParametersPanel.Controls.Add(_commonParametersUserControl);

                DomainName = DirectoryServer.DEFAULT_ACTIVE_DIRECTORY_GLOBAL_CATALOG_DOMAIN_NAME;
            }
            else
            {
                _domainNameLabel.Enabled = _theDomainNameRequiredLabel.Enabled = _theDomainNameTextBox.Enabled = true;
                _theCommonParametersPanel.Controls.Clear();
                _theCommonParametersPanel.Controls.Add(_commonParametersGrid);

                if (!IsEditDialog)
                {
                    DomainName = "";
                }
                else
                {
                    DomainNameReadOnly = true;
                }
            }
        }

        #endregion Private methods
    }
}
