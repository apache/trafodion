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
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class SelfSignedCertificateUserControl : UserControl
    {
        private ConnectionDefinition _connectionDefinition;
        public readonly List<int> KeySizes = new List<int>() { 1024, 2048 };
        
        public SelfSignedCertificateUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            _sscKeySizeComboBox.DataSource = KeySizes;
            _sscKeySizeComboBox.SelectedIndex = 1;
            _sscCNameTextBox.Text = aConnectionDefinition.MasterSegmentName;
            _sscOrgNameTextBox.Text = "Hewlett-Packard";
            _sscOrgUnitTextBox.Text = "Self-Signed";
            _sscCountryTextBox.MaxLength = 2;
            _sscOrgNameTextBox.Select();
            UpdateControls();
        }

        private void _sscGenerateButton_Click(object sender, System.EventArgs e)
        {
            Cursor = Cursors.WaitCursor;

            try
            {
                Credentials credentials = new Credentials(_connectionDefinition);
                credentials.SystemName = _connectionDefinition.MasterSegmentName;
                credentials.CommonName = this._sscCNameTextBox.Text.Trim();
                credentials.OrgName = _sscOrgNameTextBox.Text.Trim();
                credentials.OrgUnit = _sscOrgUnitTextBox.Text.Trim();
                credentials.City = _sscCityTextBox.Text.Trim();
                credentials.State = _sscStateTextBox.Text.Trim();
                credentials.Country = _sscCountryTextBox.Text.Trim();
                credentials.KeySize = (int)_sscKeySizeComboBox.SelectedItem;

                TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.GenSelfProgressTitle, credentials, "CreateCertificate", new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error Generating Self-Signed Certificate",
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                string certificate = (string)progressDialog.ReturnValue;

                if (!string.IsNullOrEmpty(certificate))
                {
                    certificate.Replace("\r\n", Environment.NewLine);
                    certificate.Replace("\n", Environment.NewLine);

                    if (!string.IsNullOrEmpty(ConnectionDefinition.DefaultCertificateDirectory))
                    {
                        SaveFileDialog sfd = new SaveFileDialog();
                        sfd.AddExtension = true;
                        sfd.DefaultExt = "cer";
                        sfd.Filter = "Certificate File (*.cer)|*.cer";

                        sfd.InitialDirectory = ConnectionDefinition.DefaultCertificateDirectory;
                        sfd.FileName = _connectionDefinition.DefaultCertificateFileName;
                        if (sfd.ShowDialog() == DialogResult.OK)
                        {
                            using (StreamWriter sw = new StreamWriter(sfd.FileName, false))
                            {
                                sw.WriteLine(certificate);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, "Error Generating Self-Signed Certificate",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        private void UpdateControls()
        {
            _sscGenerateButton.Enabled = _sscCNameTextBox.Text.Trim().Length > 0;
        }

        private void _sscCNameTextBox_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

    }
}
