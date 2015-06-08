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
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class CACertificateUserControl : UserControl
    {
        private ConnectionDefinition _connectionDefinition;

        public readonly List<int> KeySizes = new List<int>() { 1024, 2048 };

        public CACertificateUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            _csrKeySizeComboBox.DataSource = KeySizes;
            _csrKeySizeComboBox.SelectedIndex = 1;
            _csrCNameTextBox.Text = aConnectionDefinition.MasterSegmentName;
            _caSystemNameTextBox.Text = aConnectionDefinition.MasterSegmentName;
            _csrOrgNameTextBox.Text = "Hewlett-Packard";
            _csrOrgUnitTextBox.Text = "";
            _csrCountryTextBox.MaxLength = 2;
            _caCertToolTip.SetToolTip(_certTextBox, Properties.Resources.SignedCertToolTip);
            _caCertToolTip.SetToolTip(_certLabel, Properties.Resources.SignedCertToolTip);
            _caCertToolTip.SetToolTip(_caCertTextBox, Properties.Resources.CACertToolTip);
            _caCertToolTip.SetToolTip(_caCertLabel, Properties.Resources.CACertToolTip);
            UpdateControls();
        }

        private void _csrGenerateButton_Click(object sender, System.EventArgs e)
        {
            try
            {
                Credentials credentials = new Credentials(_connectionDefinition);
                credentials.SystemName = _connectionDefinition.MasterSegmentName;
                credentials.CommonName = _csrCNameTextBox.Text.Trim();
                credentials.OrgName = _csrOrgNameTextBox.Text.Trim();
                credentials.OrgUnit = _csrOrgUnitTextBox.Text.Trim();
                credentials.City = _csrCityTextBox.Text.Trim();
                credentials.State = _csrStateTextBox.Text.Trim();
                credentials.Country = _csrCountryTextBox.Text.Trim();
                credentials.KeySize = (int)_csrKeySizeComboBox.SelectedItem;
                //string csr = credentials.GenerateCSR();

                TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.GenCSRProgressTitle, credentials, 
                            "GenerateCSR", new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, Properties.Resources.ErrorMessageTitle,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                string csr = (string)progressDialog.ReturnValue;

                if (!string.IsNullOrEmpty(csr))
                {
                    if (!string.IsNullOrEmpty(ConnectionDefinition.DefaultCertificateDirectory))
                    {
                        SaveFileDialog sfd = new SaveFileDialog();
                        sfd.AddExtension = true;
                        sfd.DefaultExt = "csr";
                        sfd.Filter = "Certificate Signing Request File (*.csr)|*.csr";
                        sfd.InitialDirectory = ConnectionDefinition.DefaultCertificateDirectory;
                        sfd.FileName = _connectionDefinition.DefaultCertificateFileName;

                        if (sfd.ShowDialog() == DialogResult.OK)
                        {
                            using (StreamWriter sw = new StreamWriter(sfd.FileName, false))
                            {
                                sw.WriteLine(csr);
                            }
                        }
                    }
                }
                else
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.InvalidCSRFromServer, Properties.Resources.ErrorMessageTitle,
                        MessageBoxButtons.OK, MessageBoxIcon.Error); 

                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.ErrorMessageTitle,
                    MessageBoxButtons.OK, MessageBoxIcon.Error); 
            }
        }

        private void _deployButton_Click(object sender, System.EventArgs e)
        {
            try
            {
                Credentials credentials = new Credentials(_connectionDefinition);
                credentials.SystemName = _connectionDefinition.MasterSegmentName;
                using (StreamReader sr = new StreamReader(_certTextBox.Text.Trim()))
                {
                    credentials.Certificate = sr.ReadToEnd();
                    if (string.IsNullOrEmpty(credentials.Certificate))
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.CertFileEmpty, Properties.Resources.ErrorMessageTitle,
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }
                }
                
                using (StreamReader sr = new StreamReader(_caCertTextBox.Text.Trim()))
                {
                    credentials.CACertificate = sr.ReadToEnd();
                    if (string.IsNullOrEmpty(credentials.CACertificate))
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.CACertFileEmpty, Properties.Resources.ErrorMessageTitle,
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }
                }

                TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.DeployCACertProgressTitle, credentials, "DeployCertificate", new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, Properties.Resources.ErrorMessageTitle,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.CACertDeploySuccessMessage, Properties.Resources.CACertificate,
                    MessageBoxButtons.OK, MessageBoxIcon.Information); 

            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.ErrorMessageTitle,
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void _certBrowseButton_Click(object sender, System.EventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.AddExtension = true;
            openFileDialog.DefaultExt = "cer";
            openFileDialog.Filter = "Certificate File (*.cer)|*.cer";
            openFileDialog.InitialDirectory = ConnectionDefinition.DefaultCertificateDirectory;
            openFileDialog.FileName = _connectionDefinition.DefaultCertificateFileName;
            openFileDialog.Title = Properties.Resources.SelectCertFile;
            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                FileInfo fileInfo = new FileInfo(openFileDialog.FileName);
                if (fileInfo.Length < 1)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.CertFileEmpty, Properties.Resources.ErrorMessageTitle,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    _certTextBox.Focus();
                }
                else
                {
                    _certTextBox.Text = openFileDialog.FileName;
                }
            }
            openFileDialog.Dispose();
        }

        private void _caCertBrowseButton_Click(object sender, System.EventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.AddExtension = true;
            openFileDialog.DefaultExt = "cer";
            openFileDialog.Filter = "Certificate File (*.cer)|*.cer";
            openFileDialog.InitialDirectory = ConnectionDefinition.DefaultCertificateDirectory;
            openFileDialog.FileName = _connectionDefinition.DefaultCertificateFileName;
            openFileDialog.Title = Properties.Resources.SelectCACertificate;
            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                FileInfo fileInfo = new FileInfo(openFileDialog.FileName);
                if (fileInfo.Length < 1)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.CACertFileEmpty, Properties.Resources.ErrorMessageTitle,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    _caCertTextBox.Focus();
                }
                else
                {
                    _caCertTextBox.Text = openFileDialog.FileName;
                }
            }
            openFileDialog.Dispose();
        }

        private void _certTextBox_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        private void _caCertTextBox_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        private void UpdateControls()
        {
            _deployButton.Enabled = (_caCertTextBox.Text.Trim().Length > 0 && _certTextBox.Text.Trim().Length > 0);
            _csrGenerateButton.Enabled = _csrCNameTextBox.Text.Trim().Length > 0;
        }

        private void _csrCNameTextBox_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

    }
}
