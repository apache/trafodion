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
using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.Framework.Connections.Controls
{
    partial class ConnectionDefinitionUserControl
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
            MyDispose();
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
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel9;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label7;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label5;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label6;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label8;
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label4;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label3;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel11;
            Trafodion.Manager.Framework.Controls.TrafodionLabel _certFileNameLabel;
            Trafodion.Manager.Framework.Controls.TrafodionLabel _theRoleLabel;
            this._theCommonConnectionProperties = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theHostTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theDefaultSchemaTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._thePortNumberTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theDriverComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionLabel7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theClearPasswordButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theClientDSCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._thePasswordTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theUserNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theConnectionNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theLiveFeedConnectionProperties = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._liveCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theLiveFeedRetryTimerTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._liveFeedPortNumberComboBox = new System.Windows.Forms.ComboBox();
            this.TrafodionLabel10 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theODBCConnectionProperties = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._certBrowseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._certFileTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theRoleNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            TrafodionLabel9 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel11 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            _certFileNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            _theRoleLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theCommonConnectionProperties.SuspendLayout();
            this._theLiveFeedConnectionProperties.SuspendLayout();
            this._theODBCConnectionProperties.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theCommonConnectionProperties
            // 
            this._theCommonConnectionProperties.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theCommonConnectionProperties.Controls.Add(this.TrafodionLabel6);
            this._theCommonConnectionProperties.Controls.Add(TrafodionLabel9);
            this._theCommonConnectionProperties.Controls.Add(this.TrafodionLabel8);
            this._theCommonConnectionProperties.Controls.Add(this._theHostTextBox);
            this._theCommonConnectionProperties.Controls.Add(this._theDefaultSchemaTextBox);
            this._theCommonConnectionProperties.Controls.Add(label7);
            this._theCommonConnectionProperties.Controls.Add(label5);
            this._theCommonConnectionProperties.Controls.Add(this.TrafodionLabel5);
            this._theCommonConnectionProperties.Controls.Add(this._thePortNumberTextBox);
            this._theCommonConnectionProperties.Controls.Add(label6);
            this._theCommonConnectionProperties.Controls.Add(this._theDriverComboBox);
            this._theCommonConnectionProperties.Controls.Add(label8);
            this._theCommonConnectionProperties.Controls.Add(this.TrafodionLabel7);
            this._theCommonConnectionProperties.Controls.Add(this._theClearPasswordButton);
            this._theCommonConnectionProperties.Controls.Add(this._theClientDSCombo);
            this._theCommonConnectionProperties.Controls.Add(TrafodionLabel1);
            this._theCommonConnectionProperties.Controls.Add(this._thePasswordTextBox);
            this._theCommonConnectionProperties.Controls.Add(label4);
            this._theCommonConnectionProperties.Controls.Add(this.TrafodionLabel4);
            this._theCommonConnectionProperties.Controls.Add(this._theUserNameTextBox);
            this._theCommonConnectionProperties.Controls.Add(label3);
            this._theCommonConnectionProperties.Controls.Add(this.TrafodionLabel3);
            this._theCommonConnectionProperties.Controls.Add(this._theConnectionNameTextBox);
            this._theCommonConnectionProperties.Controls.Add(label1);
            this._theCommonConnectionProperties.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theCommonConnectionProperties.Location = new System.Drawing.Point(13, 6);
            this._theCommonConnectionProperties.Name = "_theCommonConnectionProperties";
            this._theCommonConnectionProperties.Size = new System.Drawing.Size(623, 424);
            this._theCommonConnectionProperties.TabIndex = 2;
            this._theCommonConnectionProperties.TabStop = false;
            this._theCommonConnectionProperties.Text = "Connection Properties";
            // 
            // TrafodionLabel6
            // 
            this.TrafodionLabel6.AutoSize = true;
            this.TrafodionLabel6.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel6.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel6.Location = new System.Drawing.Point(131, 83);
            this.TrafodionLabel6.Name = "TrafodionLabel6";
            this.TrafodionLabel6.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel6.TabIndex = 35;
            this.TrafodionLabel6.Text = "*";
            // 
            // TrafodionLabel9
            // 
            TrafodionLabel9.AutoSize = true;
            TrafodionLabel9.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel9.ForeColor = System.Drawing.Color.Red;
            TrafodionLabel9.Location = new System.Drawing.Point(130, 388);
            TrafodionLabel9.Name = "TrafodionLabel9";
            TrafodionLabel9.Size = new System.Drawing.Size(117, 13);
            TrafodionLabel9.TabIndex = 25;
            TrafodionLabel9.Text = "*   Required properties";
            TrafodionLabel9.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel8
            // 
            this.TrafodionLabel8.AutoSize = true;
            this.TrafodionLabel8.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel8.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel8.Location = new System.Drawing.Point(131, 257);
            this.TrafodionLabel8.Name = "TrafodionLabel8";
            this.TrafodionLabel8.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel8.TabIndex = 15;
            this.TrafodionLabel8.Text = "*";
            // 
            // _theHostTextBox
            // 
            this._theHostTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theHostTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theHostTextBox.Location = new System.Drawing.Point(147, 78);
            this._theHostTextBox.Name = "_theHostTextBox";
            this._theHostTextBox.Size = new System.Drawing.Size(466, 20);
            this._theHostTextBox.TabIndex = 1;
            this._theHostTextBox.TextChanged += new System.EventHandler(this.TheHostTextBoxTextChanged);
            // 
            // _theDefaultSchemaTextBox
            // 
            this._theDefaultSchemaTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theDefaultSchemaTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theDefaultSchemaTextBox.Location = new System.Drawing.Point(147, 344);
            this._theDefaultSchemaTextBox.Name = "_theDefaultSchemaTextBox";
            this._theDefaultSchemaTextBox.Size = new System.Drawing.Size(466, 20);
            this._theDefaultSchemaTextBox.TabIndex = 8;
            this._theDefaultSchemaTextBox.TextChanged += new System.EventHandler(this.TheSchemaTextBoxTextChanged);
            // 
            // label7
            // 
            label7.AutoSize = true;
            label7.Font = new System.Drawing.Font("Tahoma", 8F);
            label7.Location = new System.Drawing.Point(42, 347);
            label7.Name = "label7";
            label7.Size = new System.Drawing.Size(82, 13);
            label7.TabIndex = 11;
            label7.Text = "Default Schema";
            label7.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // label5
            // 
            label5.AutoSize = true;
            label5.Font = new System.Drawing.Font("Tahoma", 8F);
            label5.Location = new System.Drawing.Point(95, 81);
            label5.Name = "label5";
            label5.Size = new System.Drawing.Size(29, 13);
            label5.TabIndex = 33;
            label5.Text = "Host";
            label5.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel5
            // 
            this.TrafodionLabel5.AutoSize = true;
            this.TrafodionLabel5.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel5.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel5.Location = new System.Drawing.Point(131, 213);
            this.TrafodionLabel5.Name = "TrafodionLabel5";
            this.TrafodionLabel5.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel5.TabIndex = 31;
            this.TrafodionLabel5.Text = "*";
            // 
            // _thePortNumberTextBox
            // 
            this._thePortNumberTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._thePortNumberTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._thePortNumberTextBox.Location = new System.Drawing.Point(147, 122);
            this._thePortNumberTextBox.MaxLength = 5;
            this._thePortNumberTextBox.Name = "_thePortNumberTextBox";
            this._thePortNumberTextBox.Size = new System.Drawing.Size(466, 20);
            this._thePortNumberTextBox.TabIndex = 2;
            this._thePortNumberTextBox.Text = "37800";
            this._thePortNumberTextBox.TextChanged += new System.EventHandler(this.ThePortNumberTextBoxTextChanged);
            // 
            // label6
            // 
            label6.AutoSize = true;
            label6.Font = new System.Drawing.Font("Tahoma", 8F);
            label6.Location = new System.Drawing.Point(26, 125);
            label6.Name = "label6";
            label6.Size = new System.Drawing.Size(98, 13);
            label6.TabIndex = 8;
            label6.Text = "ODBC Port Number";
            label6.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theDriverComboBox
            // 
            this._theDriverComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theDriverComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theDriverComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theDriverComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theDriverComboBox.FormattingEnabled = true;
            this._theDriverComboBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this._theDriverComboBox.Location = new System.Drawing.Point(147, 254);
            this._theDriverComboBox.Name = "_theDriverComboBox";
            this._theDriverComboBox.Size = new System.Drawing.Size(466, 21);
            this._theDriverComboBox.Sorted = true;
            this._theDriverComboBox.TabIndex = 6;
            this._theDriverComboBox.SelectedIndexChanged += new System.EventHandler(this.TheDriverComboBoxSelectedIndexChanged);
            // 
            // label8
            // 
            label8.AutoSize = true;
            label8.Font = new System.Drawing.Font("Tahoma", 8F);
            label8.Location = new System.Drawing.Point(88, 257);
            label8.Name = "label8";
            label8.Size = new System.Drawing.Size(36, 13);
            label8.TabIndex = 13;
            label8.Text = "Driver";
            label8.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel7
            // 
            this.TrafodionLabel7.AutoSize = true;
            this.TrafodionLabel7.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel7.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel7.Location = new System.Drawing.Point(131, 125);
            this.TrafodionLabel7.Name = "TrafodionLabel7";
            this.TrafodionLabel7.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel7.TabIndex = 10;
            this.TrafodionLabel7.Text = "*";
            // 
            // _theClearPasswordButton
            // 
            this._theClearPasswordButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theClearPasswordButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theClearPasswordButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theClearPasswordButton.Location = new System.Drawing.Point(551, 208);
            this._theClearPasswordButton.Name = "_theClearPasswordButton";
            this._theClearPasswordButton.Size = new System.Drawing.Size(66, 23);
            this._theClearPasswordButton.TabIndex = 5;
            this._theClearPasswordButton.Text = "Clea&r";
            this._theClearPasswordButton.UseVisualStyleBackColor = true;
            this._theClearPasswordButton.Click += new System.EventHandler(this.TheClearPasswordButtonClick);
            // 
            // _theClientDSCombo
            // 
            this._theClientDSCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theClientDSCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theClientDSCombo.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theClientDSCombo.FormattingEnabled = true;
            this._theClientDSCombo.ItemHeight = 13;
            this._theClientDSCombo.Location = new System.Drawing.Point(147, 299);
            this._theClientDSCombo.Name = "_theClientDSCombo";
            this._theClientDSCombo.Size = new System.Drawing.Size(466, 21);
            this._theClientDSCombo.Sorted = true;
            this._theClientDSCombo.TabIndex = 7;
            this._theClientDSCombo.SelectedIndexChanged += new System.EventHandler(this._theClientDSCombo_SelectedIndexChanged);
            // 
            // TrafodionLabel1
            // 
            TrafodionLabel1.AutoSize = true;
            TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel1.Location = new System.Drawing.Point(58, 302);
            TrafodionLabel1.Name = "TrafodionLabel1";
            TrafodionLabel1.Size = new System.Drawing.Size(66, 13);
            TrafodionLabel1.TabIndex = 2;
            TrafodionLabel1.Text = "Data Source";
            TrafodionLabel1.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _thePasswordTextBox
            // 
            this._thePasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._thePasswordTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._thePasswordTextBox.Location = new System.Drawing.Point(147, 210);
            this._thePasswordTextBox.Name = "_thePasswordTextBox";
            this._thePasswordTextBox.Size = new System.Drawing.Size(395, 20);
            this._thePasswordTextBox.TabIndex = 4;
            this._thePasswordTextBox.UseSystemPasswordChar = true;
            this._thePasswordTextBox.TextChanged += new System.EventHandler(this.ThePasswordTextBoxTextChanged);
            // 
            // label4
            // 
            label4.AutoSize = true;
            label4.Font = new System.Drawing.Font("Tahoma", 8F);
            label4.Location = new System.Drawing.Point(71, 213);
            label4.Name = "label4";
            label4.Size = new System.Drawing.Size(53, 13);
            label4.TabIndex = 30;
            label4.Text = "Password";
            label4.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel4.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel4.Location = new System.Drawing.Point(131, 169);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel4.TabIndex = 26;
            this.TrafodionLabel4.Text = "*";
            // 
            // _theUserNameTextBox
            // 
            this._theUserNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theUserNameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theUserNameTextBox.Location = new System.Drawing.Point(148, 166);
            this._theUserNameTextBox.Name = "_theUserNameTextBox";
            this._theUserNameTextBox.Size = new System.Drawing.Size(466, 20);
            this._theUserNameTextBox.TabIndex = 3;
            this._theUserNameTextBox.TextChanged += new System.EventHandler(this.TheUserIDTextBoxTextChanged);
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Font = new System.Drawing.Font("Tahoma", 8F);
            label3.Location = new System.Drawing.Point(65, 169);
            label3.Name = "label3";
            label3.Size = new System.Drawing.Size(59, 13);
            label3.TabIndex = 24;
            label3.Text = "User Name";
            label3.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel3.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel3.Location = new System.Drawing.Point(131, 38);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel3.TabIndex = 23;
            this.TrafodionLabel3.Text = "*";
            // 
            // _theConnectionNameTextBox
            // 
            this._theConnectionNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theConnectionNameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theConnectionNameTextBox.Location = new System.Drawing.Point(148, 34);
            this._theConnectionNameTextBox.MaxLength = 32;
            this._theConnectionNameTextBox.Name = "_theConnectionNameTextBox";
            this._theConnectionNameTextBox.Size = new System.Drawing.Size(466, 20);
            this._theConnectionNameTextBox.TabIndex = 0;
            this._theConnectionNameTextBox.TextChanged += new System.EventHandler(this.TheConnectionNameTextBoxTextChanged);
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Font = new System.Drawing.Font("Tahoma", 8F);
            label1.Location = new System.Drawing.Point(10, 37);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(114, 13);
            label1.TabIndex = 21;
            label1.Text = "Your Name for System";
            label1.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theLiveFeedConnectionProperties
            // 
            this._theLiveFeedConnectionProperties.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theLiveFeedConnectionProperties.Controls.Add(this._liveCheckBox);
            this._theLiveFeedConnectionProperties.Controls.Add(this._theLiveFeedRetryTimerTextBox);
            this._theLiveFeedConnectionProperties.Controls.Add(TrafodionLabel2);
            this._theLiveFeedConnectionProperties.Controls.Add(this._liveFeedPortNumberComboBox);
            this._theLiveFeedConnectionProperties.Controls.Add(this.TrafodionLabel10);
            this._theLiveFeedConnectionProperties.Controls.Add(TrafodionLabel11);
            this._theLiveFeedConnectionProperties.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theLiveFeedConnectionProperties.Location = new System.Drawing.Point(22, 513);
            this._theLiveFeedConnectionProperties.Name = "_theLiveFeedConnectionProperties";
            this._theLiveFeedConnectionProperties.Size = new System.Drawing.Size(623, 117);
            this._theLiveFeedConnectionProperties.TabIndex = 47;
            this._theLiveFeedConnectionProperties.TabStop = false;
            this._theLiveFeedConnectionProperties.Text = "Live Feed Properties";
            this._theLiveFeedConnectionProperties.Visible = false;
            // 
            // _liveCheckBox
            // 
            this._liveCheckBox.AutoSize = true;
            this._liveCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._liveCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._liveCheckBox.Location = new System.Drawing.Point(147, 82);
            this._liveCheckBox.Name = "_liveCheckBox";
            this._liveCheckBox.Size = new System.Drawing.Size(159, 18);
            this._liveCheckBox.TabIndex = 50;
            this._liveCheckBox.Text = "Connect to Live Feed Only";
            this._liveCheckBox.UseVisualStyleBackColor = true;
            // 
            // _theLiveFeedRetryTimerTextBox
            // 
            this._theLiveFeedRetryTimerTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theLiveFeedRetryTimerTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theLiveFeedRetryTimerTextBox.Location = new System.Drawing.Point(147, 49);
            this._theLiveFeedRetryTimerTextBox.Name = "_theLiveFeedRetryTimerTextBox";
            this._theLiveFeedRetryTimerTextBox.Size = new System.Drawing.Size(466, 20);
            this._theLiveFeedRetryTimerTextBox.TabIndex = 49;
            this._theLiveFeedRetryTimerTextBox.Text = "30";
            // 
            // TrafodionLabel2
            // 
            TrafodionLabel2.AutoSize = true;
            TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel2.Location = new System.Drawing.Point(33, 51);
            TrafodionLabel2.Name = "TrafodionLabel2";
            TrafodionLabel2.Size = new System.Drawing.Size(90, 13);
            TrafodionLabel2.TabIndex = 23;
            TrafodionLabel2.Text = "Retry Timer (sec)";
            TrafodionLabel2.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _liveFeedPortNumberComboBox
            // 
            this._liveFeedPortNumberComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._liveFeedPortNumberComboBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._liveFeedPortNumberComboBox.FormattingEnabled = true;
            this._liveFeedPortNumberComboBox.Items.AddRange(new object[] {
            "Default Port Number"});
            this._liveFeedPortNumberComboBox.Location = new System.Drawing.Point(147, 17);
            this._liveFeedPortNumberComboBox.Name = "_liveFeedPortNumberComboBox";
            this._liveFeedPortNumberComboBox.Size = new System.Drawing.Size(466, 21);
            this._liveFeedPortNumberComboBox.TabIndex = 48;
            this._liveFeedPortNumberComboBox.TextChanged += new System.EventHandler(this.TheLiveFeedPortNumberComboBoxTextChanged);
            // 
            // TrafodionLabel10
            // 
            this.TrafodionLabel10.AutoSize = true;
            this.TrafodionLabel10.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel10.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel10.Location = new System.Drawing.Point(128, 20);
            this.TrafodionLabel10.Name = "TrafodionLabel10";
            this.TrafodionLabel10.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel10.TabIndex = 21;
            this.TrafodionLabel10.Text = "*";
            // 
            // TrafodionLabel11
            // 
            TrafodionLabel11.AutoSize = true;
            TrafodionLabel11.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel11.Location = new System.Drawing.Point(7, 17);
            TrafodionLabel11.Name = "TrafodionLabel11";
            TrafodionLabel11.Size = new System.Drawing.Size(116, 13);
            TrafodionLabel11.TabIndex = 20;
            TrafodionLabel11.Text = "Live Feed Port Number";
            TrafodionLabel11.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theODBCConnectionProperties
            // 
            this._theODBCConnectionProperties.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theODBCConnectionProperties.Controls.Add(this._certBrowseButton);
            this._theODBCConnectionProperties.Controls.Add(this._certFileTextBox);
            this._theODBCConnectionProperties.Controls.Add(_certFileNameLabel);
            this._theODBCConnectionProperties.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theODBCConnectionProperties.Location = new System.Drawing.Point(13, 436);
            this._theODBCConnectionProperties.Name = "_theODBCConnectionProperties";
            this._theODBCConnectionProperties.Size = new System.Drawing.Size(623, 10);
            this._theODBCConnectionProperties.TabIndex = 38;
            this._theODBCConnectionProperties.TabStop = false;
            this._theODBCConnectionProperties.Text = "ODBC Properties";
            this._theODBCConnectionProperties.Visible = false;
            // 
            // _certBrowseButton
            // 
            this._certBrowseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._certBrowseButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._certBrowseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._certBrowseButton.Location = new System.Drawing.Point(534, 137);
            this._certBrowseButton.Name = "_certBrowseButton";
            this._certBrowseButton.Size = new System.Drawing.Size(66, 23);
            this._certBrowseButton.TabIndex = 38;
            this._certBrowseButton.Text = "&Browse...";
            this._certBrowseButton.UseVisualStyleBackColor = true;
            this._certBrowseButton.Visible = false;
            this._certBrowseButton.Click += new System.EventHandler(this._certBrowseButton_Click);
            // 
            // _certFileTextBox
            // 
            this._certFileTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._certFileTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._certFileTextBox.Location = new System.Drawing.Point(133, 138);
            this._certFileTextBox.Name = "_certFileTextBox";
            this._certFileTextBox.Size = new System.Drawing.Size(395, 20);
            this._certFileTextBox.TabIndex = 37;
            this._certFileTextBox.Visible = false;
            // 
            // _certFileNameLabel
            // 
            _certFileNameLabel.AutoSize = true;
            _certFileNameLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            _certFileNameLabel.Location = new System.Drawing.Point(33, 141);
            _certFileNameLabel.Name = "_certFileNameLabel";
            _certFileNameLabel.Size = new System.Drawing.Size(76, 13);
            _certFileNameLabel.TabIndex = 36;
            _certFileNameLabel.Text = "Certificate File";
            _certFileNameLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
            _certFileNameLabel.Visible = false;
            // 
            // _theRoleNameTextBox
            // 
            this._theRoleNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theRoleNameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theRoleNameTextBox.Location = new System.Drawing.Point(153, 636);
            this._theRoleNameTextBox.Name = "_theRoleNameTextBox";
            this._theRoleNameTextBox.Size = new System.Drawing.Size(466, 20);
            this._theRoleNameTextBox.TabIndex = 28;
            this._theRoleNameTextBox.Visible = false;
            // 
            // _theRoleLabel
            // 
            _theRoleLabel.AutoSize = true;
            _theRoleLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            _theRoleLabel.Location = new System.Drawing.Point(32, 639);
            _theRoleLabel.Name = "_theRoleLabel";
            _theRoleLabel.Size = new System.Drawing.Size(97, 13);
            _theRoleLabel.TabIndex = 27;
            _theRoleLabel.Text = "Primary Role Name";
            _theRoleLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
            _theRoleLabel.Visible = false;
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // ConnectionDefinitionUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theCommonConnectionProperties);
            this.Controls.Add(this._theLiveFeedConnectionProperties);
            this.Controls.Add(this._theODBCConnectionProperties);
            this.Controls.Add(this._theRoleNameTextBox);
            this.Controls.Add(_theRoleLabel);
            this.Name = "ConnectionDefinitionUserControl";
            this.Size = new System.Drawing.Size(648, 438);
            this._theCommonConnectionProperties.ResumeLayout(false);
            this._theCommonConnectionProperties.PerformLayout();
            this._theLiveFeedConnectionProperties.ResumeLayout(false);
            this._theLiveFeedConnectionProperties.PerformLayout();
            this._theODBCConnectionProperties.ResumeLayout(false);
            this._theODBCConnectionProperties.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionToolTip _theToolTip;
        private TrafodionGroupBox _theCommonConnectionProperties;
        private TrafodionButton _certBrowseButton;
        private TrafodionTextBox _certFileTextBox;
        private TrafodionLabel TrafodionLabel6;
        private TrafodionTextBox _theHostTextBox;
        private TrafodionLabel TrafodionLabel5;
        private TrafodionButton _theClearPasswordButton;
        private TrafodionTextBox _thePasswordTextBox;
        private TrafodionTextBox _theRoleNameTextBox;
        private TrafodionLabel TrafodionLabel4;
        private TrafodionTextBox _theUserNameTextBox;
        private TrafodionLabel TrafodionLabel3;
        private TrafodionTextBox _theConnectionNameTextBox;
        private TrafodionGroupBox _theLiveFeedConnectionProperties;
        private TrafodionTextBox _theLiveFeedRetryTimerTextBox;
        private System.Windows.Forms.ComboBox _liveFeedPortNumberComboBox;
        private TrafodionLabel TrafodionLabel10;
        private TrafodionGroupBox _theODBCConnectionProperties;
        private TrafodionLabel TrafodionLabel8;
        private TrafodionComboBox _theDriverComboBox;
        private TrafodionTextBox _theDefaultSchemaTextBox;
        private TrafodionLabel TrafodionLabel7;
        private TrafodionTextBox _thePortNumberTextBox;
        private TrafodionComboBox _theClientDSCombo;
        private TrafodionCheckBox _liveCheckBox;
    }
}
