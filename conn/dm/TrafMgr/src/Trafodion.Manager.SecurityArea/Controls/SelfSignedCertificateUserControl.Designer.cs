// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class SelfSignedCertificateUserControl
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
            this._sscGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.oneGuiLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscCountryTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._sscStateTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._sscCityTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._sscOrgUnitTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._sscOrgNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._sscCNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._sscGenerateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._sscKeySizeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscKeySizeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._sscCountryLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscStateLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscCityLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscOrgUnitLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscOrgNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscCNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sscPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._sscGroupBox.SuspendLayout();
            this._sscPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _sscGroupBox
            // 
            this._sscGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._sscGroupBox.Controls.Add(this.oneGuiLabel2);
            this._sscGroupBox.Controls.Add(this.oneGuiLabel1);
            this._sscGroupBox.Controls.Add(this._sscCountryTextBox);
            this._sscGroupBox.Controls.Add(this._sscStateTextBox);
            this._sscGroupBox.Controls.Add(this._sscCityTextBox);
            this._sscGroupBox.Controls.Add(this._sscOrgUnitTextBox);
            this._sscGroupBox.Controls.Add(this._sscOrgNameTextBox);
            this._sscGroupBox.Controls.Add(this._sscCNameTextBox);
            this._sscGroupBox.Controls.Add(this._sscGenerateButton);
            this._sscGroupBox.Controls.Add(this._sscKeySizeLabel);
            this._sscGroupBox.Controls.Add(this._sscKeySizeComboBox);
            this._sscGroupBox.Controls.Add(this._sscCountryLabel);
            this._sscGroupBox.Controls.Add(this._sscStateLabel);
            this._sscGroupBox.Controls.Add(this._sscCityLabel);
            this._sscGroupBox.Controls.Add(this._sscOrgUnitLabel);
            this._sscGroupBox.Controls.Add(this._sscOrgNameLabel);
            this._sscGroupBox.Controls.Add(this._sscCNameLabel);
            this._sscGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscGroupBox.Location = new System.Drawing.Point(3, 3);
            this._sscGroupBox.Name = "_sscGroupBox";
            this._sscGroupBox.Size = new System.Drawing.Size(836, 302);
            this._sscGroupBox.TabIndex = 100;
            this._sscGroupBox.TabStop = false;
            this._sscGroupBox.Text = "Generate Self-Signed Certificate";
            // 
            // oneGuiLabel2
            // 
            this.oneGuiLabel2.AutoSize = true;
            this.oneGuiLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel2.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel2.Location = new System.Drawing.Point(107, 30);
            this.oneGuiLabel2.Name = "oneGuiLabel2";
            this.oneGuiLabel2.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel2.TabIndex = 14;
            this.oneGuiLabel2.Text = "*";
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel1.Location = new System.Drawing.Point(107, 216);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel1.TabIndex = 14;
            this.oneGuiLabel1.Text = "*";
            // 
            // _sscCountryTextBox
            // 
            this._sscCountryTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._sscCountryTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscCountryTextBox.Location = new System.Drawing.Point(131, 179);
            this._sscCountryTextBox.Name = "_sscCountryTextBox";
            this._sscCountryTextBox.Size = new System.Drawing.Size(699, 21);
            this._sscCountryTextBox.TabIndex = 10;
            // 
            // _sscStateTextBox
            // 
            this._sscStateTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._sscStateTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscStateTextBox.Location = new System.Drawing.Point(131, 148);
            this._sscStateTextBox.Name = "_sscStateTextBox";
            this._sscStateTextBox.Size = new System.Drawing.Size(699, 21);
            this._sscStateTextBox.TabIndex = 8;
            // 
            // _sscCityTextBox
            // 
            this._sscCityTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._sscCityTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscCityTextBox.Location = new System.Drawing.Point(131, 117);
            this._sscCityTextBox.Name = "_sscCityTextBox";
            this._sscCityTextBox.Size = new System.Drawing.Size(699, 21);
            this._sscCityTextBox.TabIndex = 6;
            // 
            // _sscOrgUnitTextBox
            // 
            this._sscOrgUnitTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._sscOrgUnitTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscOrgUnitTextBox.Location = new System.Drawing.Point(131, 86);
            this._sscOrgUnitTextBox.Name = "_sscOrgUnitTextBox";
            this._sscOrgUnitTextBox.Size = new System.Drawing.Size(699, 21);
            this._sscOrgUnitTextBox.TabIndex = 4;
            // 
            // _sscOrgNameTextBox
            // 
            this._sscOrgNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._sscOrgNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscOrgNameTextBox.Location = new System.Drawing.Point(131, 55);
            this._sscOrgNameTextBox.Name = "_sscOrgNameTextBox";
            this._sscOrgNameTextBox.Size = new System.Drawing.Size(699, 21);
            this._sscOrgNameTextBox.TabIndex = 2;
            // 
            // _sscCNameTextBox
            // 
            this._sscCNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._sscCNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscCNameTextBox.Location = new System.Drawing.Point(131, 24);
            this._sscCNameTextBox.Name = "_sscCNameTextBox";
            this._sscCNameTextBox.Size = new System.Drawing.Size(699, 21);
            this._sscCNameTextBox.TabIndex = 1;
            this._sscCNameTextBox.TextChanged += new System.EventHandler(this._sscCNameTextBox_TextChanged);
            // 
            // _sscGenerateButton
            // 
            this._sscGenerateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscGenerateButton.Location = new System.Drawing.Point(131, 258);
            this._sscGenerateButton.Name = "_sscGenerateButton";
            this._sscGenerateButton.Size = new System.Drawing.Size(75, 23);
            this._sscGenerateButton.TabIndex = 13;
            this._sscGenerateButton.Text = "&Generate";
            this._sscGenerateButton.UseVisualStyleBackColor = true;
            this._sscGenerateButton.Click += new System.EventHandler(this._sscGenerateButton_Click);
            // 
            // _sscKeySizeLabel
            // 
            this._sscKeySizeLabel.AutoSize = true;
            this._sscKeySizeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscKeySizeLabel.Location = new System.Drawing.Point(30, 213);
            this._sscKeySizeLabel.Name = "_sscKeySizeLabel";
            this._sscKeySizeLabel.Size = new System.Drawing.Size(75, 13);
            this._sscKeySizeLabel.TabIndex = 11;
            this._sscKeySizeLabel.Text = "Key Size (bits)";
            // 
            // _sscKeySizeComboBox
            // 
            this._sscKeySizeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._sscKeySizeComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._sscKeySizeComboBox.FormattingEnabled = true;
            this._sscKeySizeComboBox.Location = new System.Drawing.Point(131, 210);
            this._sscKeySizeComboBox.Name = "_sscKeySizeComboBox";
            this._sscKeySizeComboBox.Size = new System.Drawing.Size(162, 21);
            this._sscKeySizeComboBox.TabIndex = 12;
            // 
            // _sscCountryLabel
            // 
            this._sscCountryLabel.AutoSize = true;
            this._sscCountryLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscCountryLabel.Location = new System.Drawing.Point(16, 183);
            this._sscCountryLabel.Name = "_sscCountryLabel";
            this._sscCountryLabel.Size = new System.Drawing.Size(89, 13);
            this._sscCountryLabel.TabIndex = 9;
            this._sscCountryLabel.Text = "Country / Region";
            // 
            // _sscStateLabel
            // 
            this._sscStateLabel.AutoSize = true;
            this._sscStateLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscStateLabel.Location = new System.Drawing.Point(21, 150);
            this._sscStateLabel.Name = "_sscStateLabel";
            this._sscStateLabel.Size = new System.Drawing.Size(84, 13);
            this._sscStateLabel.TabIndex = 7;
            this._sscStateLabel.Text = "State / Province";
            // 
            // _sscCityLabel
            // 
            this._sscCityLabel.AutoSize = true;
            this._sscCityLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscCityLabel.Location = new System.Drawing.Point(33, 120);
            this._sscCityLabel.Name = "_sscCityLabel";
            this._sscCityLabel.Size = new System.Drawing.Size(72, 13);
            this._sscCityLabel.TabIndex = 5;
            this._sscCityLabel.Text = "City / Locality";
            // 
            // _sscOrgUnitLabel
            // 
            this._sscOrgUnitLabel.AutoSize = true;
            this._sscOrgUnitLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscOrgUnitLabel.Location = new System.Drawing.Point(7, 90);
            this._sscOrgUnitLabel.Name = "_sscOrgUnitLabel";
            this._sscOrgUnitLabel.Size = new System.Drawing.Size(98, 13);
            this._sscOrgUnitLabel.TabIndex = 3;
            this._sscOrgUnitLabel.Text = "Organizational Unit";
            // 
            // _sscOrgNameLabel
            // 
            this._sscOrgNameLabel.AutoSize = true;
            this._sscOrgNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscOrgNameLabel.Location = new System.Drawing.Point(37, 58);
            this._sscOrgNameLabel.Name = "_sscOrgNameLabel";
            this._sscOrgNameLabel.Size = new System.Drawing.Size(68, 13);
            this._sscOrgNameLabel.TabIndex = 1;
            this._sscOrgNameLabel.Text = "Organization";
            // 
            // _sscCNameLabel
            // 
            this._sscCNameLabel.AutoSize = true;
            this._sscCNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sscCNameLabel.Location = new System.Drawing.Point(24, 27);
            this._sscCNameLabel.Name = "_sscCNameLabel";
            this._sscCNameLabel.Size = new System.Drawing.Size(78, 13);
            this._sscCNameLabel.TabIndex = 0;
            this._sscCNameLabel.Text = "Common Name";
            // 
            // _sscPanel
            // 
            this._sscPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._sscPanel.Controls.Add(this._sscGroupBox);
            this._sscPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._sscPanel.Location = new System.Drawing.Point(0, 0);
            this._sscPanel.Name = "_sscPanel";
            this._sscPanel.Size = new System.Drawing.Size(842, 515);
            this._sscPanel.TabIndex = 9;
            // 
            // SelfSignedCertificateUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._sscPanel);
            this.Name = "SelfSignedCertificateUserControl";
            this.Size = new System.Drawing.Size(842, 515);
            this._sscGroupBox.ResumeLayout(false);
            this._sscGroupBox.PerformLayout();
            this._sscPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _sscGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sscCountryTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sscStateTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sscCityTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sscOrgUnitTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sscOrgNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _sscCNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _sscGenerateButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sscKeySizeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _sscKeySizeComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sscCountryLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sscStateLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sscCityLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sscOrgUnitLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sscOrgNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sscCNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _sscPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
    }
}
