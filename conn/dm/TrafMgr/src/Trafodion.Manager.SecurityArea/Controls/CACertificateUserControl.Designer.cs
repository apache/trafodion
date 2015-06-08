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
namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class CACertificateUserControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this._caCertificateTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._csrTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._csrGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.oneGuiLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._csrCountryTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._csrStateTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._csrCityTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._csrOrgUnitTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._csrOrgNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._csrCNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._csrGenerateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._csrKeySizeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._csrKeySizeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._csrCountryLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._csrStateLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._csrCityLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._csrOrgUnitLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._csrOrgNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._csrCNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._deployCertificateTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._caDeployGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.oneGuiLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._caSystemNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._caSystemNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._deployButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._certTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._caCertBrowseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._certBrowseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._caCertLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._certLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._caCertTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiButton2 = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._caCertToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._caCertificateTabControl.SuspendLayout();
            this._csrTabPage.SuspendLayout();
            this._csrGroupBox.SuspendLayout();
            this._deployCertificateTabPage.SuspendLayout();
            this._caDeployGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _caCertificateTabControl
            // 
            this._caCertificateTabControl.Controls.Add(this._csrTabPage);
            this._caCertificateTabControl.Controls.Add(this._deployCertificateTabPage);
            this._caCertificateTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._caCertificateTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caCertificateTabControl.HotTrack = true;
            this._caCertificateTabControl.Location = new System.Drawing.Point(0, 0);
            this._caCertificateTabControl.Multiline = true;
            this._caCertificateTabControl.Name = "_caCertificateTabControl";
            this._caCertificateTabControl.Padding = new System.Drawing.Point(10, 5);
            this._caCertificateTabControl.SelectedIndex = 0;
            this._caCertificateTabControl.Size = new System.Drawing.Size(862, 500);
            this._caCertificateTabControl.TabIndex = 0;
            // 
            // _csrTabPage
            // 
            this._csrTabPage.Controls.Add(this._csrGroupBox);
            this._csrTabPage.Location = new System.Drawing.Point(4, 26);
            this._csrTabPage.Name = "_csrTabPage";
            this._csrTabPage.Padding = new System.Windows.Forms.Padding(3);
            this._csrTabPage.Size = new System.Drawing.Size(854, 470);
            this._csrTabPage.TabIndex = 0;
            this._csrTabPage.Text = "Generate CSR";
            this._csrTabPage.UseVisualStyleBackColor = true;
            // 
            // _csrGroupBox
            // 
            this._csrGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._csrGroupBox.Controls.Add(this.oneGuiLabel3);
            this._csrGroupBox.Controls.Add(this.oneGuiLabel1);
            this._csrGroupBox.Controls.Add(this._csrCountryTextBox);
            this._csrGroupBox.Controls.Add(this._csrStateTextBox);
            this._csrGroupBox.Controls.Add(this._csrCityTextBox);
            this._csrGroupBox.Controls.Add(this._csrOrgUnitTextBox);
            this._csrGroupBox.Controls.Add(this._csrOrgNameTextBox);
            this._csrGroupBox.Controls.Add(this._csrCNameTextBox);
            this._csrGroupBox.Controls.Add(this._csrGenerateButton);
            this._csrGroupBox.Controls.Add(this._csrKeySizeLabel);
            this._csrGroupBox.Controls.Add(this._csrKeySizeComboBox);
            this._csrGroupBox.Controls.Add(this._csrCountryLabel);
            this._csrGroupBox.Controls.Add(this._csrStateLabel);
            this._csrGroupBox.Controls.Add(this._csrCityLabel);
            this._csrGroupBox.Controls.Add(this._csrOrgUnitLabel);
            this._csrGroupBox.Controls.Add(this._csrOrgNameLabel);
            this._csrGroupBox.Controls.Add(this._csrCNameLabel);
            this._csrGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrGroupBox.Location = new System.Drawing.Point(4, 6);
            this._csrGroupBox.Name = "_csrGroupBox";
            this._csrGroupBox.Size = new System.Drawing.Size(845, 314);
            this._csrGroupBox.TabIndex = 100;
            this._csrGroupBox.TabStop = false;
            this._csrGroupBox.Text = "Generate Certificate Signing Request";
            // 
            // oneGuiLabel3
            // 
            this.oneGuiLabel3.AutoSize = true;
            this.oneGuiLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel3.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel3.Location = new System.Drawing.Point(107, 216);
            this.oneGuiLabel3.Name = "oneGuiLabel3";
            this.oneGuiLabel3.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel3.TabIndex = 15;
            this.oneGuiLabel3.Text = "*";
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel1.Location = new System.Drawing.Point(107, 29);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel1.TabIndex = 15;
            this.oneGuiLabel1.Text = "*";
            // 
            // _csrCountryTextBox
            // 
            this._csrCountryTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._csrCountryTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrCountryTextBox.Location = new System.Drawing.Point(129, 179);
            this._csrCountryTextBox.Name = "_csrCountryTextBox";
            this._csrCountryTextBox.Size = new System.Drawing.Size(710, 21);
            this._csrCountryTextBox.TabIndex = 11;
            // 
            // _csrStateTextBox
            // 
            this._csrStateTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._csrStateTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrStateTextBox.Location = new System.Drawing.Point(129, 148);
            this._csrStateTextBox.Name = "_csrStateTextBox";
            this._csrStateTextBox.Size = new System.Drawing.Size(710, 21);
            this._csrStateTextBox.TabIndex = 9;
            // 
            // _csrCityTextBox
            // 
            this._csrCityTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._csrCityTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrCityTextBox.Location = new System.Drawing.Point(129, 117);
            this._csrCityTextBox.Name = "_csrCityTextBox";
            this._csrCityTextBox.Size = new System.Drawing.Size(710, 21);
            this._csrCityTextBox.TabIndex = 7;
            // 
            // _csrOrgUnitTextBox
            // 
            this._csrOrgUnitTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._csrOrgUnitTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrOrgUnitTextBox.Location = new System.Drawing.Point(129, 86);
            this._csrOrgUnitTextBox.Name = "_csrOrgUnitTextBox";
            this._csrOrgUnitTextBox.Size = new System.Drawing.Size(710, 21);
            this._csrOrgUnitTextBox.TabIndex = 5;
            // 
            // _csrOrgNameTextBox
            // 
            this._csrOrgNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._csrOrgNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrOrgNameTextBox.Location = new System.Drawing.Point(129, 55);
            this._csrOrgNameTextBox.Name = "_csrOrgNameTextBox";
            this._csrOrgNameTextBox.Size = new System.Drawing.Size(710, 21);
            this._csrOrgNameTextBox.TabIndex = 3;
            // 
            // _csrCNameTextBox
            // 
            this._csrCNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._csrCNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrCNameTextBox.Location = new System.Drawing.Point(129, 24);
            this._csrCNameTextBox.Name = "_csrCNameTextBox";
            this._csrCNameTextBox.Size = new System.Drawing.Size(710, 21);
            this._csrCNameTextBox.TabIndex = 1;
            this._csrCNameTextBox.TextChanged += new System.EventHandler(this._csrCNameTextBox_TextChanged);
            // 
            // _csrGenerateButton
            // 
            this._csrGenerateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrGenerateButton.Location = new System.Drawing.Point(129, 272);
            this._csrGenerateButton.Name = "_csrGenerateButton";
            this._csrGenerateButton.Size = new System.Drawing.Size(75, 23);
            this._csrGenerateButton.TabIndex = 14;
            this._csrGenerateButton.Text = "&Generate";
            this._csrGenerateButton.UseVisualStyleBackColor = true;
            this._csrGenerateButton.Click += new System.EventHandler(this._csrGenerateButton_Click);
            // 
            // _csrKeySizeLabel
            // 
            this._csrKeySizeLabel.AutoSize = true;
            this._csrKeySizeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrKeySizeLabel.Location = new System.Drawing.Point(30, 213);
            this._csrKeySizeLabel.Name = "_csrKeySizeLabel";
            this._csrKeySizeLabel.Size = new System.Drawing.Size(75, 13);
            this._csrKeySizeLabel.TabIndex = 12;
            this._csrKeySizeLabel.Text = "Key Size (bits)";
            // 
            // _csrKeySizeComboBox
            // 
            this._csrKeySizeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._csrKeySizeComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._csrKeySizeComboBox.FormattingEnabled = true;
            this._csrKeySizeComboBox.Location = new System.Drawing.Point(129, 210);
            this._csrKeySizeComboBox.Name = "_csrKeySizeComboBox";
            this._csrKeySizeComboBox.Size = new System.Drawing.Size(194, 21);
            this._csrKeySizeComboBox.TabIndex = 13;
            // 
            // _csrCountryLabel
            // 
            this._csrCountryLabel.AutoSize = true;
            this._csrCountryLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrCountryLabel.Location = new System.Drawing.Point(16, 182);
            this._csrCountryLabel.Name = "_csrCountryLabel";
            this._csrCountryLabel.Size = new System.Drawing.Size(89, 13);
            this._csrCountryLabel.TabIndex = 10;
            this._csrCountryLabel.Text = "Country / Region";
            // 
            // _csrStateLabel
            // 
            this._csrStateLabel.AutoSize = true;
            this._csrStateLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrStateLabel.Location = new System.Drawing.Point(21, 151);
            this._csrStateLabel.Name = "_csrStateLabel";
            this._csrStateLabel.Size = new System.Drawing.Size(84, 13);
            this._csrStateLabel.TabIndex = 8;
            this._csrStateLabel.Text = "State / Province";
            // 
            // _csrCityLabel
            // 
            this._csrCityLabel.AutoSize = true;
            this._csrCityLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrCityLabel.Location = new System.Drawing.Point(33, 120);
            this._csrCityLabel.Name = "_csrCityLabel";
            this._csrCityLabel.Size = new System.Drawing.Size(72, 13);
            this._csrCityLabel.TabIndex = 6;
            this._csrCityLabel.Text = "City / Locality";
            // 
            // _csrOrgUnitLabel
            // 
            this._csrOrgUnitLabel.AutoSize = true;
            this._csrOrgUnitLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrOrgUnitLabel.Location = new System.Drawing.Point(7, 90);
            this._csrOrgUnitLabel.Name = "_csrOrgUnitLabel";
            this._csrOrgUnitLabel.Size = new System.Drawing.Size(98, 13);
            this._csrOrgUnitLabel.TabIndex = 4;
            this._csrOrgUnitLabel.Text = "Organizational Unit";
            // 
            // _csrOrgNameLabel
            // 
            this._csrOrgNameLabel.AutoSize = true;
            this._csrOrgNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrOrgNameLabel.Location = new System.Drawing.Point(37, 58);
            this._csrOrgNameLabel.Name = "_csrOrgNameLabel";
            this._csrOrgNameLabel.Size = new System.Drawing.Size(68, 13);
            this._csrOrgNameLabel.TabIndex = 2;
            this._csrOrgNameLabel.Text = "Organization";
            // 
            // _csrCNameLabel
            // 
            this._csrCNameLabel.AutoSize = true;
            this._csrCNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._csrCNameLabel.Location = new System.Drawing.Point(27, 27);
            this._csrCNameLabel.Name = "_csrCNameLabel";
            this._csrCNameLabel.Size = new System.Drawing.Size(78, 13);
            this._csrCNameLabel.TabIndex = 0;
            this._csrCNameLabel.Text = "Common Name";
            // 
            // _deployCertificateTabPage
            // 
            this._deployCertificateTabPage.Controls.Add(this._caDeployGroupBox);
            this._deployCertificateTabPage.Location = new System.Drawing.Point(4, 26);
            this._deployCertificateTabPage.Name = "_deployCertificateTabPage";
            this._deployCertificateTabPage.Padding = new System.Windows.Forms.Padding(3);
            this._deployCertificateTabPage.Size = new System.Drawing.Size(854, 470);
            this._deployCertificateTabPage.TabIndex = 1;
            this._deployCertificateTabPage.Text = "Deploy CA Signed Certificate";
            this._deployCertificateTabPage.UseVisualStyleBackColor = true;
            // 
            // _caDeployGroupBox
            // 
            this._caDeployGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._caDeployGroupBox.Controls.Add(this.oneGuiLabel6);
            this._caDeployGroupBox.Controls.Add(this.oneGuiLabel5);
            this._caDeployGroupBox.Controls.Add(this.oneGuiLabel4);
            this._caDeployGroupBox.Controls.Add(this._caSystemNameTextBox);
            this._caDeployGroupBox.Controls.Add(this._caSystemNameLabel);
            this._caDeployGroupBox.Controls.Add(this._deployButton);
            this._caDeployGroupBox.Controls.Add(this._certTextBox);
            this._caDeployGroupBox.Controls.Add(this._caCertBrowseButton);
            this._caDeployGroupBox.Controls.Add(this._certBrowseButton);
            this._caDeployGroupBox.Controls.Add(this._caCertLabel);
            this._caDeployGroupBox.Controls.Add(this._certLabel);
            this._caDeployGroupBox.Controls.Add(this._caCertTextBox);
            this._caDeployGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caDeployGroupBox.Location = new System.Drawing.Point(4, 3);
            this._caDeployGroupBox.Name = "_caDeployGroupBox";
            this._caDeployGroupBox.Size = new System.Drawing.Size(842, 184);
            this._caDeployGroupBox.TabIndex = 100;
            this._caDeployGroupBox.TabStop = false;
            this._caDeployGroupBox.Text = "Deploy CA Signed Certificate";
            // 
            // oneGuiLabel6
            // 
            this.oneGuiLabel6.AutoSize = true;
            this.oneGuiLabel6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel6.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel6.Location = new System.Drawing.Point(132, 107);
            this.oneGuiLabel6.Name = "oneGuiLabel6";
            this.oneGuiLabel6.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel6.TabIndex = 9;
            this.oneGuiLabel6.Text = "*";
            // 
            // oneGuiLabel5
            // 
            this.oneGuiLabel5.AutoSize = true;
            this.oneGuiLabel5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel5.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel5.Location = new System.Drawing.Point(132, 69);
            this.oneGuiLabel5.Name = "oneGuiLabel5";
            this.oneGuiLabel5.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel5.TabIndex = 9;
            this.oneGuiLabel5.Text = "*";
            // 
            // oneGuiLabel4
            // 
            this.oneGuiLabel4.AutoSize = true;
            this.oneGuiLabel4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel4.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel4.Location = new System.Drawing.Point(132, 31);
            this.oneGuiLabel4.Name = "oneGuiLabel4";
            this.oneGuiLabel4.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel4.TabIndex = 9;
            this.oneGuiLabel4.Text = "*";
            // 
            // _caSystemNameTextBox
            // 
            this._caSystemNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._caSystemNameTextBox.Enabled = false;
            this._caSystemNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caSystemNameTextBox.Location = new System.Drawing.Point(148, 25);
            this._caSystemNameTextBox.Name = "_caSystemNameTextBox";
            this._caSystemNameTextBox.Size = new System.Drawing.Size(602, 21);
            this._caSystemNameTextBox.TabIndex = 1;
            // 
            // _caSystemNameLabel
            // 
            this._caSystemNameLabel.AutoSize = true;
            this._caSystemNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caSystemNameLabel.Location = new System.Drawing.Point(6, 28);
            this._caSystemNameLabel.Name = "_caSystemNameLabel";
            this._caSystemNameLabel.Size = new System.Drawing.Size(72, 13);
            this._caSystemNameLabel.TabIndex = 0;
            this._caSystemNameLabel.Text = "System Name";
            // 
            // _deployButton
            // 
            this._deployButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._deployButton.Location = new System.Drawing.Point(148, 144);
            this._deployButton.Name = "_deployButton";
            this._deployButton.Size = new System.Drawing.Size(75, 23);
            this._deployButton.TabIndex = 8;
            this._deployButton.Text = "&Deploy";
            this._deployButton.UseVisualStyleBackColor = true;
            this._deployButton.Click += new System.EventHandler(this._deployButton_Click);
            // 
            // _certTextBox
            // 
            this._certTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._certTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._certTextBox.Location = new System.Drawing.Point(148, 63);
            this._certTextBox.Name = "_certTextBox";
            this._certTextBox.Size = new System.Drawing.Size(602, 21);
            this._certTextBox.TabIndex = 3;
            this._certTextBox.TextChanged += new System.EventHandler(this._certTextBox_TextChanged);
            // 
            // _caCertBrowseButton
            // 
            this._caCertBrowseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._caCertBrowseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caCertBrowseButton.Location = new System.Drawing.Point(756, 100);
            this._caCertBrowseButton.Name = "_caCertBrowseButton";
            this._caCertBrowseButton.Size = new System.Drawing.Size(75, 23);
            this._caCertBrowseButton.TabIndex = 7;
            this._caCertBrowseButton.Text = "B&rowse...";
            this._caCertBrowseButton.UseVisualStyleBackColor = true;
            this._caCertBrowseButton.Click += new System.EventHandler(this._caCertBrowseButton_Click);
            // 
            // _certBrowseButton
            // 
            this._certBrowseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._certBrowseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._certBrowseButton.Location = new System.Drawing.Point(756, 62);
            this._certBrowseButton.Name = "_certBrowseButton";
            this._certBrowseButton.Size = new System.Drawing.Size(75, 23);
            this._certBrowseButton.TabIndex = 4;
            this._certBrowseButton.Text = "&Browse...";
            this._certBrowseButton.UseVisualStyleBackColor = true;
            this._certBrowseButton.Click += new System.EventHandler(this._certBrowseButton_Click);
            // 
            // _caCertLabel
            // 
            this._caCertLabel.AutoSize = true;
            this._caCertLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caCertLabel.Location = new System.Drawing.Point(6, 104);
            this._caCertLabel.Name = "_caCertLabel";
            this._caCertLabel.Size = new System.Drawing.Size(122, 13);
            this._caCertLabel.TabIndex = 5;
            this._caCertLabel.Text = "Intermediate Certificate";
            // 
            // _certLabel
            // 
            this._certLabel.AutoSize = true;
            this._certLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._certLabel.Location = new System.Drawing.Point(6, 66);
            this._certLabel.Name = "_certLabel";
            this._certLabel.Size = new System.Drawing.Size(83, 13);
            this._certLabel.TabIndex = 2;
            this._certLabel.Text = "Root Certificate";
            // 
            // _caCertTextBox
            // 
            this._caCertTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._caCertTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._caCertTextBox.Location = new System.Drawing.Point(148, 101);
            this._caCertTextBox.Name = "_caCertTextBox";
            this._caCertTextBox.Size = new System.Drawing.Size(602, 21);
            this._caCertTextBox.TabIndex = 6;
            this._caCertTextBox.TextChanged += new System.EventHandler(this._caCertTextBox_TextChanged);
            // 
            // oneGuiButton2
            // 
            this.oneGuiButton2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiButton2.Location = new System.Drawing.Point(12, 106);
            this.oneGuiButton2.Name = "oneGuiButton2";
            this.oneGuiButton2.Size = new System.Drawing.Size(75, 23);
            this.oneGuiButton2.TabIndex = 6;
            this.oneGuiButton2.Text = "&Generate";
            this.oneGuiButton2.UseVisualStyleBackColor = true;
            // 
            // _caCertToolTip
            // 
            this._caCertToolTip.IsBalloon = true;
            // 
            // CACertificateUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._caCertificateTabControl);
            this.Name = "CACertificateUserControl";
            this.Size = new System.Drawing.Size(862, 500);
            this._caCertificateTabControl.ResumeLayout(false);
            this._csrTabPage.ResumeLayout(false);
            this._csrGroupBox.ResumeLayout(false);
            this._csrGroupBox.PerformLayout();
            this._deployCertificateTabPage.ResumeLayout(false);
            this._caDeployGroupBox.ResumeLayout(false);
            this._caDeployGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _caCertificateTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _csrTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _deployCertificateTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _csrGenerateButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _csrKeySizeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _csrKeySizeComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _caDeployGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _deployButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _certTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _caCertBrowseButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _certBrowseButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _caCertLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _certLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _caCertTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _csrGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton oneGuiButton2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _csrCNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _csrCNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _csrCountryTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _csrStateTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _csrCityTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _csrOrgUnitTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _csrOrgNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _csrCountryLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _csrStateLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _csrCityLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _csrOrgUnitLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _csrOrgNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _caSystemNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _caSystemNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _caCertToolTip;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel3;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel6;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel5;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel4;
    }
}
