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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class LDAPConnectionConfigDialog
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
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.saveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.defaultsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.loadServerCerButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.browseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theLabelCertContent = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.certificateRichTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.cerLocationTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theLabelServerCertFile = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.refreshTimeNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.defaultSectionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theLabelDefaultSection = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.backgroundWorker = new System.ComponentModel.BackgroundWorker();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.toolStripProgressBar1 = new System.Windows.Forms.ToolStripProgressBar();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.sectionTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.tabPageEnterprise = new System.Windows.Forms.TabPage();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.portTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theLabelLDAPHostName = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.attributesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.configAttributeSettingControl = new Trafodion.Manager.OverviewArea.Controls.ConfigAttributeSettingControl();
            this.connectionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theLabelNetTimeout = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelPReserveConn = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelLDAPTimeout = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.hostNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.networkTimeoutNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.encryptTypeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.retryCountNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._theLabelEncryptType = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelRetryCount = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelConfirmPWD = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.ldapTimeoutNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.timelimitNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._theLabelRetryDelay = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.searchDNTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.confirmPasswdMaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theLabelSearchDN = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelPwd = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.retryDelayNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._theLabelTimeLimit = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelLDAPPort = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.passwdMaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.tabPageCluster = new System.Windows.Forms.TabPage();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.portTextBox1 = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.searchDNTextBox1 = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theLabelSearchDN2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.confirmPasswdMaskedTextBox1 = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.passwdMaskedTextBox1 = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theLabelConfirmPWD2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelPWD2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.encryptTypeComboBox1 = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theLabelEncryptionType = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.connectionComboBox1 = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theLabelPreserveConn2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.hostNameTextBox1 = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theLabelLDAPHostName2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.attributes1GroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.configAttributeSettingControl1 = new Trafodion.Manager.OverviewArea.Controls.ConfigAttributeSettingControl();
            this.retryCountNumericUpDown1 = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.retryDelayNumericUpDown1 = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.ldapTimeoutNumericUpDown1 = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.networkTimeoutNumericUpDown1 = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.timelimitNumericUpDown1 = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._theLabelLDAPPort2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelNetTimeout2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelTimeLimit2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelRetryDelay2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelRetryCount2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabelLDAPTimeout2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.sectionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.refreshTimeCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.remoteCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.localCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.configurationToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theTrafodionPanelBottom = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTrafodionPanelContent = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.defaultsGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.refreshTimeNumericUpDown)).BeginInit();
            this.statusStrip.SuspendLayout();
            this.sectionTabControl.SuspendLayout();
            this.tabPageEnterprise.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.attributesGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.networkTimeoutNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.retryCountNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.ldapTimeoutNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.timelimitNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.retryDelayNumericUpDown)).BeginInit();
            this.tabPageCluster.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.attributes1GroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.retryCountNumericUpDown1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.retryDelayNumericUpDown1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.ldapTimeoutNumericUpDown1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.networkTimeoutNumericUpDown1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.timelimitNumericUpDown1)).BeginInit();
            this.sectionGroupBox.SuspendLayout();
            this._theTrafodionPanelBottom.SuspendLayout();
            this._theTrafodionPanelContent.SuspendLayout();
            this.SuspendLayout();
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(433, 6);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(96, 23);
            this.cancelButton.TabIndex = 39;
            this.cancelButton.Text = "&Close";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // saveButton
            // 
            this.saveButton.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.saveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.saveButton.Location = new System.Drawing.Point(326, 6);
            this.saveButton.Name = "saveButton";
            this.saveButton.Size = new System.Drawing.Size(96, 23);
            this.saveButton.TabIndex = 38;
            this.saveButton.Text = "&Save";
            this.saveButton.UseVisualStyleBackColor = true;
            this.saveButton.Click += new System.EventHandler(this.saveButton_Click);
            // 
            // defaultsGroupBox
            // 
            this.defaultsGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.defaultsGroupBox.Controls.Add(this.loadServerCerButton);
            this.defaultsGroupBox.Controls.Add(this.browseButton);
            this.defaultsGroupBox.Controls.Add(this._theLabelCertContent);
            this.defaultsGroupBox.Controls.Add(this.certificateRichTextBox);
            this.defaultsGroupBox.Controls.Add(this.cerLocationTextBox);
            this.defaultsGroupBox.Controls.Add(this._theLabelServerCertFile);
            this.defaultsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.defaultsGroupBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.defaultsGroupBox.Location = new System.Drawing.Point(11, 471);
            this.defaultsGroupBox.Name = "defaultsGroupBox";
            this.defaultsGroupBox.Size = new System.Drawing.Size(625, 114);
            this.defaultsGroupBox.TabIndex = 2;
            this.defaultsGroupBox.TabStop = false;
            this.defaultsGroupBox.Text = "CA Certificate Attributes";
            // 
            // loadServerCerButton
            // 
            this.loadServerCerButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.loadServerCerButton.Image = global::Trafodion.Manager.OverviewArea.Properties.Resources.DownArrowIcon;
            this.loadServerCerButton.Location = new System.Drawing.Point(554, 13);
            this.loadServerCerButton.Name = "loadServerCerButton";
            this.loadServerCerButton.Size = new System.Drawing.Size(35, 23);
            this.loadServerCerButton.TabIndex = 38;
            this.loadServerCerButton.UseVisualStyleBackColor = true;
            this.loadServerCerButton.Click += new System.EventHandler(this.loadServerCerButton_Click);
            // 
            // browseButton
            // 
            this.browseButton.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.browseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.browseButton.Image = global::Trafodion.Manager.OverviewArea.Properties.Resources.fileopen;
            this.browseButton.Location = new System.Drawing.Point(554, 42);
            this.browseButton.Name = "browseButton";
            this.browseButton.Size = new System.Drawing.Size(65, 37);
            this.browseButton.TabIndex = 37;
            this.browseButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageAboveText;
            this.browseButton.UseVisualStyleBackColor = true;
            this.browseButton.Click += new System.EventHandler(this.browseButton_Click);
            // 
            // _theLabelCertContent
            // 
            this._theLabelCertContent.AutoSize = true;
            this._theLabelCertContent.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelCertContent.Location = new System.Drawing.Point(9, 42);
            this._theLabelCertContent.Name = "_theLabelCertContent";
            this._theLabelCertContent.Size = new System.Drawing.Size(99, 13);
            this._theLabelCertContent.TabIndex = 7;
            this._theLabelCertContent.Text = "Certificate Content";
            // 
            // certificateRichTextBox
            // 
            this.certificateRichTextBox.BackColor = System.Drawing.Color.LightGray;
            this.certificateRichTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.certificateRichTextBox.Location = new System.Drawing.Point(132, 42);
            this.certificateRichTextBox.Name = "certificateRichTextBox";
            this.certificateRichTextBox.ReadOnly = true;
            this.certificateRichTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.certificateRichTextBox.Size = new System.Drawing.Size(415, 66);
            this.certificateRichTextBox.TabIndex = 36;
            this.certificateRichTextBox.TabStop = false;
            this.certificateRichTextBox.Text = "";
            // 
            // cerLocationTextBox
            // 
            this.cerLocationTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.cerLocationTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cerLocationTextBox.Location = new System.Drawing.Point(132, 15);
            this.cerLocationTextBox.Name = "cerLocationTextBox";
            this.cerLocationTextBox.Size = new System.Drawing.Size(415, 21);
            this.cerLocationTextBox.TabIndex = 33;
            this.cerLocationTextBox.TextChanged += new System.EventHandler(this.cerLocationTextBox_TextChanged);
            this.cerLocationTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.cerLocationTextBox_KeyPress);
            // 
            // _theLabelServerCertFile
            // 
            this._theLabelServerCertFile.AutoSize = true;
            this._theLabelServerCertFile.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelServerCertFile.Location = new System.Drawing.Point(9, 19);
            this._theLabelServerCertFile.Name = "_theLabelServerCertFile";
            this._theLabelServerCertFile.Size = new System.Drawing.Size(93, 13);
            this._theLabelServerCertFile.TabIndex = 2;
            this._theLabelServerCertFile.Text = "CA Certificate File";
            // 
            // refreshTimeNumericUpDown
            // 
            this.refreshTimeNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshTimeNumericUpDown.Location = new System.Drawing.Point(476, 16);
            this.refreshTimeNumericUpDown.Maximum = new decimal(new int[] {
            1440,
            0,
            0,
            0});
            this.refreshTimeNumericUpDown.Name = "refreshTimeNumericUpDown";
            this.refreshTimeNumericUpDown.Size = new System.Drawing.Size(143, 21);
            this.refreshTimeNumericUpDown.TabIndex = 35;
            this.refreshTimeNumericUpDown.Value = new decimal(new int[] {
            30,
            0,
            0,
            0});
            // 
            // defaultSectionComboBox
            // 
            this.defaultSectionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.defaultSectionComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.defaultSectionComboBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.defaultSectionComboBox.FormattingEnabled = true;
            this.defaultSectionComboBox.Location = new System.Drawing.Point(155, 39);
            this.defaultSectionComboBox.Name = "defaultSectionComboBox";
            this.defaultSectionComboBox.Size = new System.Drawing.Size(143, 21);
            this.defaultSectionComboBox.TabIndex = 32;
            // 
            // _theLabelDefaultSection
            // 
            this._theLabelDefaultSection.AutoSize = true;
            this._theLabelDefaultSection.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelDefaultSection.Location = new System.Drawing.Point(20, 43);
            this._theLabelDefaultSection.Name = "_theLabelDefaultSection";
            this._theLabelDefaultSection.Size = new System.Drawing.Size(110, 13);
            this._theLabelDefaultSection.TabIndex = 8;
            this._theLabelDefaultSection.Text = "Default Configuration";
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(540, 6);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(96, 23);
            this.helpButton.TabIndex = 40;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // refreshButton
            // 
            this.refreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshButton.Location = new System.Drawing.Point(11, 4);
            this.refreshButton.Name = "refreshButton";
            this.refreshButton.Size = new System.Drawing.Size(96, 23);
            this.refreshButton.TabIndex = 37;
            this.refreshButton.Text = "&Refresh";
            this.refreshButton.UseVisualStyleBackColor = true;
            this.refreshButton.Click += new System.EventHandler(this.refreshButton_Click);
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripProgressBar1,
            this.toolStripStatusLabel1});
            this.statusStrip.Location = new System.Drawing.Point(0, 620);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(648, 22);
            this.statusStrip.SizingGrip = false;
            this.statusStrip.TabIndex = 7;
            this.statusStrip.Text = "statusStrip1";
            // 
            // toolStripProgressBar1
            // 
            this.toolStripProgressBar1.Name = "toolStripProgressBar1";
            this.toolStripProgressBar1.Size = new System.Drawing.Size(300, 16);
            this.toolStripProgressBar1.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(118, 17);
            this.toolStripStatusLabel1.Text = "toolStripStatusLabel1";
            // 
            // sectionTabControl
            // 
            this.sectionTabControl.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.sectionTabControl.Controls.Add(this.tabPageEnterprise);
            this.sectionTabControl.Controls.Add(this.tabPageCluster);
            this.sectionTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sectionTabControl.HotTrack = true;
            this.sectionTabControl.Location = new System.Drawing.Point(11, 72);
            this.sectionTabControl.Multiline = true;
            this.sectionTabControl.Name = "sectionTabControl";
            this.sectionTabControl.Padding = new System.Drawing.Point(10, 5);
            this.sectionTabControl.SelectedIndex = 0;
            this.sectionTabControl.Size = new System.Drawing.Size(629, 396);
            this.sectionTabControl.TabIndex = 41;
            this.sectionTabControl.SelectedIndexChanged += new System.EventHandler(this.sectionTabControl_SelectedIndexChanged);
            this.sectionTabControl.Selecting += new System.Windows.Forms.TabControlCancelEventHandler(this.sectionTabControl_Selecting);
            // 
            // tabPageEnterprise
            // 
            this.tabPageEnterprise.Controls.Add(this.TrafodionPanel1);
            this.tabPageEnterprise.Location = new System.Drawing.Point(4, 26);
            this.tabPageEnterprise.Name = "tabPageEnterprise";
            this.tabPageEnterprise.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageEnterprise.Size = new System.Drawing.Size(621, 366);
            this.tabPageEnterprise.TabIndex = 0;
            this.tabPageEnterprise.Text = "Enterprise";
            this.tabPageEnterprise.UseVisualStyleBackColor = true;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel4);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel2);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel7);
            this.TrafodionPanel1.Controls.Add(this.portTextBox);
            this.TrafodionPanel1.Controls.Add(this._theLabelLDAPHostName);
            this.TrafodionPanel1.Controls.Add(this.attributesGroupBox);
            this.TrafodionPanel1.Controls.Add(this.connectionComboBox);
            this.TrafodionPanel1.Controls.Add(this._theLabelNetTimeout);
            this.TrafodionPanel1.Controls.Add(this._theLabelPReserveConn);
            this.TrafodionPanel1.Controls.Add(this._theLabelLDAPTimeout);
            this.TrafodionPanel1.Controls.Add(this.hostNameTextBox);
            this.TrafodionPanel1.Controls.Add(this.networkTimeoutNumericUpDown);
            this.TrafodionPanel1.Controls.Add(this.encryptTypeComboBox);
            this.TrafodionPanel1.Controls.Add(this.retryCountNumericUpDown);
            this.TrafodionPanel1.Controls.Add(this._theLabelEncryptType);
            this.TrafodionPanel1.Controls.Add(this._theLabelRetryCount);
            this.TrafodionPanel1.Controls.Add(this._theLabelConfirmPWD);
            this.TrafodionPanel1.Controls.Add(this.ldapTimeoutNumericUpDown);
            this.TrafodionPanel1.Controls.Add(this.timelimitNumericUpDown);
            this.TrafodionPanel1.Controls.Add(this._theLabelRetryDelay);
            this.TrafodionPanel1.Controls.Add(this.searchDNTextBox);
            this.TrafodionPanel1.Controls.Add(this.confirmPasswdMaskedTextBox);
            this.TrafodionPanel1.Controls.Add(this._theLabelSearchDN);
            this.TrafodionPanel1.Controls.Add(this._theLabelPwd);
            this.TrafodionPanel1.Controls.Add(this.retryDelayNumericUpDown);
            this.TrafodionPanel1.Controls.Add(this._theLabelTimeLimit);
            this.TrafodionPanel1.Controls.Add(this._theLabelLDAPPort);
            this.TrafodionPanel1.Controls.Add(this.passwdMaskedTextBox);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(3, 3);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(615, 360);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel4.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel4.Location = new System.Drawing.Point(442, 43);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel4.TabIndex = 73;
            this.TrafodionLabel4.Text = "*";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel2.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel2.Location = new System.Drawing.Point(110, 43);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel2.TabIndex = 72;
            this.TrafodionLabel2.Text = "*";
            // 
            // TrafodionLabel7
            // 
            this.TrafodionLabel7.AutoSize = true;
            this.TrafodionLabel7.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel7.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel7.Location = new System.Drawing.Point(110, 10);
            this.TrafodionLabel7.Name = "TrafodionLabel7";
            this.TrafodionLabel7.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel7.TabIndex = 36;
            this.TrafodionLabel7.Text = "*";
            // 
            // portTextBox
            // 
            this.portTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.portTextBox.Location = new System.Drawing.Point(133, 39);
            this.portTextBox.MaxLength = 5;
            this.portTextBox.Name = "portTextBox";
            this.portTextBox.Size = new System.Drawing.Size(143, 21);
            this.portTextBox.TabIndex = 70;
            // 
            // _theLabelLDAPHostName
            // 
            this._theLabelLDAPHostName.AutoSize = true;
            this._theLabelLDAPHostName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelLDAPHostName.Location = new System.Drawing.Point(7, 10);
            this._theLabelLDAPHostName.Name = "_theLabelLDAPHostName";
            this._theLabelLDAPHostName.Size = new System.Drawing.Size(87, 13);
            this._theLabelLDAPHostName.TabIndex = 64;
            this._theLabelLDAPHostName.Text = "LDAP Host Name";
            // 
            // attributesGroupBox
            // 
            this.attributesGroupBox.Controls.Add(this.configAttributeSettingControl);
            this.attributesGroupBox.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.attributesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.attributesGroupBox.Location = new System.Drawing.Point(0, 222);
            this.attributesGroupBox.Name = "attributesGroupBox";
            this.attributesGroupBox.Size = new System.Drawing.Size(615, 138);
            this.attributesGroupBox.TabIndex = 41;
            this.attributesGroupBox.TabStop = false;
            this.attributesGroupBox.Text = "Attributes";
            // 
            // configAttributeSettingControl
            // 
            this.configAttributeSettingControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.configAttributeSettingControl.Location = new System.Drawing.Point(3, 17);
            this.configAttributeSettingControl.Name = "configAttributeSettingControl";
            this.configAttributeSettingControl.OnClickHandler = null;
            this.configAttributeSettingControl.Size = new System.Drawing.Size(609, 118);
            this.configAttributeSettingControl.TabIndex = 15;
            // 
            // connectionComboBox
            // 
            this.connectionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.connectionComboBox.Enabled = false;
            this.connectionComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.connectionComboBox.FormattingEnabled = true;
            this.connectionComboBox.Location = new System.Drawing.Point(465, 195);
            this.connectionComboBox.Name = "connectionComboBox";
            this.connectionComboBox.Size = new System.Drawing.Size(143, 21);
            this.connectionComboBox.TabIndex = 69;
            this.connectionComboBox.Visible = false;
            // 
            // _theLabelNetTimeout
            // 
            this._theLabelNetTimeout.AutoSize = true;
            this._theLabelNetTimeout.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelNetTimeout.Location = new System.Drawing.Point(7, 147);
            this._theLabelNetTimeout.Name = "_theLabelNetTimeout";
            this._theLabelNetTimeout.Size = new System.Drawing.Size(120, 13);
            this._theLabelNetTimeout.TabIndex = 31;
            this._theLabelNetTimeout.Text = "Network Timeout (secs)";
            // 
            // _theLabelPReserveConn
            // 
            this._theLabelPReserveConn.AutoSize = true;
            this._theLabelPReserveConn.Enabled = false;
            this._theLabelPReserveConn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelPReserveConn.Location = new System.Drawing.Point(336, 199);
            this._theLabelPReserveConn.Name = "_theLabelPReserveConn";
            this._theLabelPReserveConn.Size = new System.Drawing.Size(107, 13);
            this._theLabelPReserveConn.TabIndex = 68;
            this._theLabelPReserveConn.Text = "Preserve Connection";
            this._theLabelPReserveConn.Visible = false;
            // 
            // _theLabelLDAPTimeout
            // 
            this._theLabelLDAPTimeout.AutoSize = true;
            this._theLabelLDAPTimeout.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelLDAPTimeout.Location = new System.Drawing.Point(7, 173);
            this._theLabelLDAPTimeout.Name = "_theLabelLDAPTimeout";
            this._theLabelLDAPTimeout.Size = new System.Drawing.Size(105, 13);
            this._theLabelLDAPTimeout.TabIndex = 59;
            this._theLabelLDAPTimeout.Text = "LDAP Timeout (secs)";
            // 
            // hostNameTextBox
            // 
            this.hostNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.hostNameTextBox.Location = new System.Drawing.Point(133, 2);
            this.hostNameTextBox.Multiline = true;
            this.hostNameTextBox.Name = "hostNameTextBox";
            this.hostNameTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.hostNameTextBox.Size = new System.Drawing.Size(475, 32);
            this.hostNameTextBox.TabIndex = 65;
            this.hostNameTextBox.Tag = "";
            // 
            // networkTimeoutNumericUpDown
            // 
            this.networkTimeoutNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.networkTimeoutNumericUpDown.Location = new System.Drawing.Point(133, 143);
            this.networkTimeoutNumericUpDown.Maximum = new decimal(new int[] {
            3600,
            0,
            0,
            0});
            this.networkTimeoutNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.networkTimeoutNumericUpDown.Name = "networkTimeoutNumericUpDown";
            this.networkTimeoutNumericUpDown.Size = new System.Drawing.Size(143, 21);
            this.networkTimeoutNumericUpDown.TabIndex = 5;
            this.networkTimeoutNumericUpDown.Value = new decimal(new int[] {
            30,
            0,
            0,
            0});
            this.networkTimeoutNumericUpDown.Leave += new System.EventHandler(this.networkTimeoutNumericUpDown_Leave);
            // 
            // encryptTypeComboBox
            // 
            this.encryptTypeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.encryptTypeComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.encryptTypeComboBox.FormattingEnabled = true;
            this.encryptTypeComboBox.Location = new System.Drawing.Point(465, 39);
            this.encryptTypeComboBox.Name = "encryptTypeComboBox";
            this.encryptTypeComboBox.Size = new System.Drawing.Size(143, 21);
            this.encryptTypeComboBox.TabIndex = 67;
            this.encryptTypeComboBox.SelectedIndexChanged += new System.EventHandler(this.encryptTypeComboBox_SelectedIndexChanged);
            // 
            // retryCountNumericUpDown
            // 
            this.retryCountNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.retryCountNumericUpDown.Location = new System.Drawing.Point(465, 169);
            this.retryCountNumericUpDown.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.retryCountNumericUpDown.Name = "retryCountNumericUpDown";
            this.retryCountNumericUpDown.Size = new System.Drawing.Size(143, 21);
            this.retryCountNumericUpDown.TabIndex = 6;
            this.retryCountNumericUpDown.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            // 
            // _theLabelEncryptType
            // 
            this._theLabelEncryptType.AutoSize = true;
            this._theLabelEncryptType.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelEncryptType.Location = new System.Drawing.Point(336, 43);
            this._theLabelEncryptType.Name = "_theLabelEncryptType";
            this._theLabelEncryptType.Size = new System.Drawing.Size(97, 13);
            this._theLabelEncryptType.TabIndex = 66;
            this._theLabelEncryptType.Text = "Encryption Method";
            // 
            // _theLabelRetryCount
            // 
            this._theLabelRetryCount.AutoSize = true;
            this._theLabelRetryCount.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelRetryCount.Location = new System.Drawing.Point(336, 173);
            this._theLabelRetryCount.Name = "_theLabelRetryCount";
            this._theLabelRetryCount.Size = new System.Drawing.Size(66, 13);
            this._theLabelRetryCount.TabIndex = 54;
            this._theLabelRetryCount.Text = "Retry Count";
            // 
            // _theLabelConfirmPWD
            // 
            this._theLabelConfirmPWD.AutoSize = true;
            this._theLabelConfirmPWD.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelConfirmPWD.Location = new System.Drawing.Point(7, 121);
            this._theLabelConfirmPWD.Name = "_theLabelConfirmPWD";
            this._theLabelConfirmPWD.Size = new System.Drawing.Size(93, 13);
            this._theLabelConfirmPWD.TabIndex = 4;
            this._theLabelConfirmPWD.Text = "Confirm Password";
            // 
            // ldapTimeoutNumericUpDown
            // 
            this.ldapTimeoutNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ldapTimeoutNumericUpDown.Location = new System.Drawing.Point(133, 169);
            this.ldapTimeoutNumericUpDown.Maximum = new decimal(new int[] {
            3600,
            0,
            0,
            0});
            this.ldapTimeoutNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.ldapTimeoutNumericUpDown.Name = "ldapTimeoutNumericUpDown";
            this.ldapTimeoutNumericUpDown.Size = new System.Drawing.Size(143, 21);
            this.ldapTimeoutNumericUpDown.TabIndex = 7;
            this.ldapTimeoutNumericUpDown.Value = new decimal(new int[] {
            30,
            0,
            0,
            0});
            this.ldapTimeoutNumericUpDown.Leave += new System.EventHandler(this.ldapTimeoutNumericUpDown_Leave);
            // 
            // timelimitNumericUpDown
            // 
            this.timelimitNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.timelimitNumericUpDown.Location = new System.Drawing.Point(133, 195);
            this.timelimitNumericUpDown.Maximum = new decimal(new int[] {
            3600,
            0,
            0,
            0});
            this.timelimitNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.timelimitNumericUpDown.Name = "timelimitNumericUpDown";
            this.timelimitNumericUpDown.Size = new System.Drawing.Size(143, 21);
            this.timelimitNumericUpDown.TabIndex = 11;
            this.timelimitNumericUpDown.Value = new decimal(new int[] {
            30,
            0,
            0,
            0});
            // 
            // _theLabelRetryDelay
            // 
            this._theLabelRetryDelay.AutoSize = true;
            this._theLabelRetryDelay.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelRetryDelay.Location = new System.Drawing.Point(336, 147);
            this._theLabelRetryDelay.Name = "_theLabelRetryDelay";
            this._theLabelRetryDelay.Size = new System.Drawing.Size(96, 13);
            this._theLabelRetryDelay.TabIndex = 61;
            this._theLabelRetryDelay.Text = "Retry Delay (secs)";
            // 
            // searchDNTextBox
            // 
            this.searchDNTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.searchDNTextBox.Location = new System.Drawing.Point(133, 65);
            this.searchDNTextBox.MaxLength = 128;
            this.searchDNTextBox.Name = "searchDNTextBox";
            this.searchDNTextBox.Size = new System.Drawing.Size(475, 21);
            this.searchDNTextBox.TabIndex = 12;
            // 
            // confirmPasswdMaskedTextBox
            // 
            this.confirmPasswdMaskedTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.confirmPasswdMaskedTextBox.Location = new System.Drawing.Point(133, 117);
            this.confirmPasswdMaskedTextBox.MaxLength = 128;
            this.confirmPasswdMaskedTextBox.Name = "confirmPasswdMaskedTextBox";
            this.confirmPasswdMaskedTextBox.Size = new System.Drawing.Size(475, 21);
            this.confirmPasswdMaskedTextBox.TabIndex = 14;
            this.confirmPasswdMaskedTextBox.UseSystemPasswordChar = true;
            // 
            // _theLabelSearchDN
            // 
            this._theLabelSearchDN.AutoSize = true;
            this._theLabelSearchDN.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelSearchDN.Location = new System.Drawing.Point(7, 69);
            this._theLabelSearchDN.Name = "_theLabelSearchDN";
            this._theLabelSearchDN.Size = new System.Drawing.Size(57, 13);
            this._theLabelSearchDN.TabIndex = 50;
            this._theLabelSearchDN.Text = "Search DN";
            // 
            // _theLabelPwd
            // 
            this._theLabelPwd.AutoSize = true;
            this._theLabelPwd.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelPwd.Location = new System.Drawing.Point(7, 95);
            this._theLabelPwd.Name = "_theLabelPwd";
            this._theLabelPwd.Size = new System.Drawing.Size(53, 13);
            this._theLabelPwd.TabIndex = 3;
            this._theLabelPwd.Text = "Password";
            // 
            // retryDelayNumericUpDown
            // 
            this.retryDelayNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.retryDelayNumericUpDown.Location = new System.Drawing.Point(465, 143);
            this.retryDelayNumericUpDown.Maximum = new decimal(new int[] {
            300,
            0,
            0,
            0});
            this.retryDelayNumericUpDown.Name = "retryDelayNumericUpDown";
            this.retryDelayNumericUpDown.Size = new System.Drawing.Size(143, 21);
            this.retryDelayNumericUpDown.TabIndex = 8;
            this.retryDelayNumericUpDown.Value = new decimal(new int[] {
            2,
            0,
            0,
            0});
            // 
            // _theLabelTimeLimit
            // 
            this._theLabelTimeLimit.AutoSize = true;
            this._theLabelTimeLimit.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelTimeLimit.Location = new System.Drawing.Point(7, 199);
            this._theLabelTimeLimit.Name = "_theLabelTimeLimit";
            this._theLabelTimeLimit.Size = new System.Drawing.Size(85, 13);
            this._theLabelTimeLimit.TabIndex = 63;
            this._theLabelTimeLimit.Text = "Time Limit (secs)";
            // 
            // _theLabelLDAPPort
            // 
            this._theLabelLDAPPort.AutoSize = true;
            this._theLabelLDAPPort.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelLDAPPort.Location = new System.Drawing.Point(7, 43);
            this._theLabelLDAPPort.Name = "_theLabelLDAPPort";
            this._theLabelLDAPPort.Size = new System.Drawing.Size(55, 13);
            this._theLabelLDAPPort.TabIndex = 21;
            this._theLabelLDAPPort.Text = "LDAP Port";
            // 
            // passwdMaskedTextBox
            // 
            this.passwdMaskedTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.passwdMaskedTextBox.Location = new System.Drawing.Point(133, 91);
            this.passwdMaskedTextBox.MaxLength = 128;
            this.passwdMaskedTextBox.Name = "passwdMaskedTextBox";
            this.passwdMaskedTextBox.Size = new System.Drawing.Size(475, 21);
            this.passwdMaskedTextBox.TabIndex = 13;
            this.passwdMaskedTextBox.UseSystemPasswordChar = true;
            // 
            // tabPageCluster
            // 
            this.tabPageCluster.Controls.Add(this.TrafodionPanel2);
            this.tabPageCluster.Location = new System.Drawing.Point(4, 26);
            this.tabPageCluster.Name = "tabPageCluster";
            this.tabPageCluster.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageCluster.Size = new System.Drawing.Size(621, 366);
            this.tabPageCluster.TabIndex = 1;
            this.tabPageCluster.Text = "Cluster";
            this.tabPageCluster.UseVisualStyleBackColor = true;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel5);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel6);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel8);
            this.TrafodionPanel2.Controls.Add(this.portTextBox1);
            this.TrafodionPanel2.Controls.Add(this.searchDNTextBox1);
            this.TrafodionPanel2.Controls.Add(this._theLabelSearchDN2);
            this.TrafodionPanel2.Controls.Add(this.confirmPasswdMaskedTextBox1);
            this.TrafodionPanel2.Controls.Add(this.passwdMaskedTextBox1);
            this.TrafodionPanel2.Controls.Add(this._theLabelConfirmPWD2);
            this.TrafodionPanel2.Controls.Add(this._theLabelPWD2);
            this.TrafodionPanel2.Controls.Add(this.encryptTypeComboBox1);
            this.TrafodionPanel2.Controls.Add(this._theLabelEncryptionType);
            this.TrafodionPanel2.Controls.Add(this.connectionComboBox1);
            this.TrafodionPanel2.Controls.Add(this._theLabelPreserveConn2);
            this.TrafodionPanel2.Controls.Add(this.hostNameTextBox1);
            this.TrafodionPanel2.Controls.Add(this._theLabelLDAPHostName2);
            this.TrafodionPanel2.Controls.Add(this.attributes1GroupBox);
            this.TrafodionPanel2.Controls.Add(this.retryCountNumericUpDown1);
            this.TrafodionPanel2.Controls.Add(this.retryDelayNumericUpDown1);
            this.TrafodionPanel2.Controls.Add(this.ldapTimeoutNumericUpDown1);
            this.TrafodionPanel2.Controls.Add(this.networkTimeoutNumericUpDown1);
            this.TrafodionPanel2.Controls.Add(this.timelimitNumericUpDown1);
            this.TrafodionPanel2.Controls.Add(this._theLabelLDAPPort2);
            this.TrafodionPanel2.Controls.Add(this._theLabelNetTimeout2);
            this.TrafodionPanel2.Controls.Add(this._theLabelTimeLimit2);
            this.TrafodionPanel2.Controls.Add(this._theLabelRetryDelay2);
            this.TrafodionPanel2.Controls.Add(this._theLabelRetryCount2);
            this.TrafodionPanel2.Controls.Add(this._theLabelLDAPTimeout2);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel2.Location = new System.Drawing.Point(3, 3);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(615, 360);
            this.TrafodionPanel2.TabIndex = 0;
            // 
            // TrafodionLabel5
            // 
            this.TrafodionLabel5.AutoSize = true;
            this.TrafodionLabel5.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel5.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel5.Location = new System.Drawing.Point(442, 43);
            this.TrafodionLabel5.Name = "TrafodionLabel5";
            this.TrafodionLabel5.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel5.TabIndex = 110;
            this.TrafodionLabel5.Text = "*";
            // 
            // TrafodionLabel6
            // 
            this.TrafodionLabel6.AutoSize = true;
            this.TrafodionLabel6.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel6.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel6.Location = new System.Drawing.Point(110, 43);
            this.TrafodionLabel6.Name = "TrafodionLabel6";
            this.TrafodionLabel6.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel6.TabIndex = 109;
            this.TrafodionLabel6.Text = "*";
            // 
            // TrafodionLabel8
            // 
            this.TrafodionLabel8.AutoSize = true;
            this.TrafodionLabel8.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel8.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel8.Location = new System.Drawing.Point(110, 10);
            this.TrafodionLabel8.Name = "TrafodionLabel8";
            this.TrafodionLabel8.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel8.TabIndex = 108;
            this.TrafodionLabel8.Text = "*";
            // 
            // portTextBox1
            // 
            this.portTextBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.portTextBox1.Location = new System.Drawing.Point(133, 39);
            this.portTextBox1.MaxLength = 5;
            this.portTextBox1.Name = "portTextBox1";
            this.portTextBox1.Size = new System.Drawing.Size(143, 21);
            this.portTextBox1.TabIndex = 107;
            // 
            // searchDNTextBox1
            // 
            this.searchDNTextBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.searchDNTextBox1.Location = new System.Drawing.Point(133, 65);
            this.searchDNTextBox1.MaxLength = 128;
            this.searchDNTextBox1.Name = "searchDNTextBox1";
            this.searchDNTextBox1.Size = new System.Drawing.Size(475, 21);
            this.searchDNTextBox1.TabIndex = 103;
            // 
            // _theLabelSearchDN2
            // 
            this._theLabelSearchDN2.AutoSize = true;
            this._theLabelSearchDN2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelSearchDN2.Location = new System.Drawing.Point(7, 69);
            this._theLabelSearchDN2.Name = "_theLabelSearchDN2";
            this._theLabelSearchDN2.Size = new System.Drawing.Size(57, 13);
            this._theLabelSearchDN2.TabIndex = 106;
            this._theLabelSearchDN2.Text = "Search DN";
            // 
            // confirmPasswdMaskedTextBox1
            // 
            this.confirmPasswdMaskedTextBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.confirmPasswdMaskedTextBox1.Location = new System.Drawing.Point(133, 117);
            this.confirmPasswdMaskedTextBox1.MaxLength = 128;
            this.confirmPasswdMaskedTextBox1.Name = "confirmPasswdMaskedTextBox1";
            this.confirmPasswdMaskedTextBox1.Size = new System.Drawing.Size(475, 21);
            this.confirmPasswdMaskedTextBox1.TabIndex = 105;
            this.confirmPasswdMaskedTextBox1.UseSystemPasswordChar = true;
            // 
            // passwdMaskedTextBox1
            // 
            this.passwdMaskedTextBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.passwdMaskedTextBox1.Location = new System.Drawing.Point(133, 91);
            this.passwdMaskedTextBox1.MaxLength = 128;
            this.passwdMaskedTextBox1.Name = "passwdMaskedTextBox1";
            this.passwdMaskedTextBox1.Size = new System.Drawing.Size(475, 21);
            this.passwdMaskedTextBox1.TabIndex = 104;
            this.passwdMaskedTextBox1.UseSystemPasswordChar = true;
            // 
            // _theLabelConfirmPWD2
            // 
            this._theLabelConfirmPWD2.AutoSize = true;
            this._theLabelConfirmPWD2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelConfirmPWD2.Location = new System.Drawing.Point(7, 121);
            this._theLabelConfirmPWD2.Name = "_theLabelConfirmPWD2";
            this._theLabelConfirmPWD2.Size = new System.Drawing.Size(93, 13);
            this._theLabelConfirmPWD2.TabIndex = 102;
            this._theLabelConfirmPWD2.Text = "Confirm Password";
            // 
            // _theLabelPWD2
            // 
            this._theLabelPWD2.AutoSize = true;
            this._theLabelPWD2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelPWD2.Location = new System.Drawing.Point(7, 95);
            this._theLabelPWD2.Name = "_theLabelPWD2";
            this._theLabelPWD2.Size = new System.Drawing.Size(53, 13);
            this._theLabelPWD2.TabIndex = 101;
            this._theLabelPWD2.Text = "Password";
            // 
            // encryptTypeComboBox1
            // 
            this.encryptTypeComboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.encryptTypeComboBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.encryptTypeComboBox1.FormattingEnabled = true;
            this.encryptTypeComboBox1.Location = new System.Drawing.Point(465, 39);
            this.encryptTypeComboBox1.Name = "encryptTypeComboBox1";
            this.encryptTypeComboBox1.Size = new System.Drawing.Size(143, 21);
            this.encryptTypeComboBox1.TabIndex = 100;
            this.encryptTypeComboBox1.SelectedIndexChanged += new System.EventHandler(this.encryptTypeComboBox_SelectedIndexChanged);
            // 
            // _theLabelEncryptionType
            // 
            this._theLabelEncryptionType.AutoSize = true;
            this._theLabelEncryptionType.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelEncryptionType.Location = new System.Drawing.Point(336, 43);
            this._theLabelEncryptionType.Name = "_theLabelEncryptionType";
            this._theLabelEncryptionType.Size = new System.Drawing.Size(97, 13);
            this._theLabelEncryptionType.TabIndex = 99;
            this._theLabelEncryptionType.Text = "Encryption Method";
            // 
            // connectionComboBox1
            // 
            this.connectionComboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.connectionComboBox1.Enabled = false;
            this.connectionComboBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.connectionComboBox1.FormattingEnabled = true;
            this.connectionComboBox1.Location = new System.Drawing.Point(465, 195);
            this.connectionComboBox1.Name = "connectionComboBox1";
            this.connectionComboBox1.Size = new System.Drawing.Size(143, 21);
            this.connectionComboBox1.TabIndex = 98;
            this.connectionComboBox1.Visible = false;
            // 
            // _theLabelPreserveConn2
            // 
            this._theLabelPreserveConn2.AutoSize = true;
            this._theLabelPreserveConn2.Enabled = false;
            this._theLabelPreserveConn2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelPreserveConn2.Location = new System.Drawing.Point(336, 199);
            this._theLabelPreserveConn2.Name = "_theLabelPreserveConn2";
            this._theLabelPreserveConn2.Size = new System.Drawing.Size(107, 13);
            this._theLabelPreserveConn2.TabIndex = 97;
            this._theLabelPreserveConn2.Text = "Preserve Connection";
            this._theLabelPreserveConn2.Visible = false;
            // 
            // hostNameTextBox1
            // 
            this.hostNameTextBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.hostNameTextBox1.Location = new System.Drawing.Point(133, 2);
            this.hostNameTextBox1.Multiline = true;
            this.hostNameTextBox1.Name = "hostNameTextBox1";
            this.hostNameTextBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.hostNameTextBox1.Size = new System.Drawing.Size(475, 32);
            this.hostNameTextBox1.TabIndex = 96;
            // 
            // _theLabelLDAPHostName2
            // 
            this._theLabelLDAPHostName2.AutoSize = true;
            this._theLabelLDAPHostName2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelLDAPHostName2.Location = new System.Drawing.Point(7, 10);
            this._theLabelLDAPHostName2.Name = "_theLabelLDAPHostName2";
            this._theLabelLDAPHostName2.Size = new System.Drawing.Size(87, 13);
            this._theLabelLDAPHostName2.TabIndex = 95;
            this._theLabelLDAPHostName2.Text = "LDAP Host Name";
            // 
            // attributes1GroupBox
            // 
            this.attributes1GroupBox.Controls.Add(this.configAttributeSettingControl1);
            this.attributes1GroupBox.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.attributes1GroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.attributes1GroupBox.Location = new System.Drawing.Point(0, 222);
            this.attributes1GroupBox.Name = "attributes1GroupBox";
            this.attributes1GroupBox.Size = new System.Drawing.Size(615, 138);
            this.attributes1GroupBox.TabIndex = 94;
            this.attributes1GroupBox.TabStop = false;
            this.attributes1GroupBox.Text = "Attributes";
            // 
            // configAttributeSettingControl1
            // 
            this.configAttributeSettingControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.configAttributeSettingControl1.Location = new System.Drawing.Point(3, 17);
            this.configAttributeSettingControl1.Name = "configAttributeSettingControl1";
            this.configAttributeSettingControl1.OnClickHandler = null;
            this.configAttributeSettingControl1.Size = new System.Drawing.Size(609, 118);
            this.configAttributeSettingControl1.TabIndex = 31;
            // 
            // retryCountNumericUpDown1
            // 
            this.retryCountNumericUpDown1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.retryCountNumericUpDown1.Location = new System.Drawing.Point(465, 169);
            this.retryCountNumericUpDown1.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.retryCountNumericUpDown1.Name = "retryCountNumericUpDown1";
            this.retryCountNumericUpDown1.Size = new System.Drawing.Size(143, 21);
            this.retryCountNumericUpDown1.TabIndex = 85;
            this.retryCountNumericUpDown1.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            // 
            // retryDelayNumericUpDown1
            // 
            this.retryDelayNumericUpDown1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.retryDelayNumericUpDown1.Location = new System.Drawing.Point(465, 143);
            this.retryDelayNumericUpDown1.Maximum = new decimal(new int[] {
            300,
            0,
            0,
            0});
            this.retryDelayNumericUpDown1.Name = "retryDelayNumericUpDown1";
            this.retryDelayNumericUpDown1.Size = new System.Drawing.Size(143, 21);
            this.retryDelayNumericUpDown1.TabIndex = 89;
            this.retryDelayNumericUpDown1.Value = new decimal(new int[] {
            2,
            0,
            0,
            0});
            // 
            // ldapTimeoutNumericUpDown1
            // 
            this.ldapTimeoutNumericUpDown1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.ldapTimeoutNumericUpDown1.Location = new System.Drawing.Point(133, 169);
            this.ldapTimeoutNumericUpDown1.Maximum = new decimal(new int[] {
            3600,
            0,
            0,
            0});
            this.ldapTimeoutNumericUpDown1.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.ldapTimeoutNumericUpDown1.Name = "ldapTimeoutNumericUpDown1";
            this.ldapTimeoutNumericUpDown1.Size = new System.Drawing.Size(143, 21);
            this.ldapTimeoutNumericUpDown1.TabIndex = 86;
            this.ldapTimeoutNumericUpDown1.Value = new decimal(new int[] {
            30,
            0,
            0,
            0});
            this.ldapTimeoutNumericUpDown1.Leave += new System.EventHandler(this.ldapTimeoutNumericUpDown1_Leave);
            // 
            // networkTimeoutNumericUpDown1
            // 
            this.networkTimeoutNumericUpDown1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.networkTimeoutNumericUpDown1.Location = new System.Drawing.Point(133, 143);
            this.networkTimeoutNumericUpDown1.Maximum = new decimal(new int[] {
            3600,
            0,
            0,
            0});
            this.networkTimeoutNumericUpDown1.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.networkTimeoutNumericUpDown1.Name = "networkTimeoutNumericUpDown1";
            this.networkTimeoutNumericUpDown1.Size = new System.Drawing.Size(143, 21);
            this.networkTimeoutNumericUpDown1.TabIndex = 83;
            this.networkTimeoutNumericUpDown1.Value = new decimal(new int[] {
            30,
            0,
            0,
            0});
            this.networkTimeoutNumericUpDown1.Leave += new System.EventHandler(this.networkTimeoutNumericUpDown1_Leave);
            // 
            // timelimitNumericUpDown1
            // 
            this.timelimitNumericUpDown1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.timelimitNumericUpDown1.Location = new System.Drawing.Point(133, 195);
            this.timelimitNumericUpDown1.Maximum = new decimal(new int[] {
            3600,
            0,
            0,
            0});
            this.timelimitNumericUpDown1.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.timelimitNumericUpDown1.Name = "timelimitNumericUpDown1";
            this.timelimitNumericUpDown1.Size = new System.Drawing.Size(143, 21);
            this.timelimitNumericUpDown1.TabIndex = 91;
            this.timelimitNumericUpDown1.Value = new decimal(new int[] {
            30,
            0,
            0,
            0});
            // 
            // _theLabelLDAPPort2
            // 
            this._theLabelLDAPPort2.AutoSize = true;
            this._theLabelLDAPPort2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelLDAPPort2.Location = new System.Drawing.Point(7, 43);
            this._theLabelLDAPPort2.Name = "_theLabelLDAPPort2";
            this._theLabelLDAPPort2.Size = new System.Drawing.Size(55, 13);
            this._theLabelLDAPPort2.TabIndex = 84;
            this._theLabelLDAPPort2.Text = "LDAP Port";
            // 
            // _theLabelNetTimeout2
            // 
            this._theLabelNetTimeout2.AutoSize = true;
            this._theLabelNetTimeout2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelNetTimeout2.Location = new System.Drawing.Point(7, 147);
            this._theLabelNetTimeout2.Name = "_theLabelNetTimeout2";
            this._theLabelNetTimeout2.Size = new System.Drawing.Size(120, 13);
            this._theLabelNetTimeout2.TabIndex = 93;
            this._theLabelNetTimeout2.Text = "Network Timeout (secs)";
            // 
            // _theLabelTimeLimit2
            // 
            this._theLabelTimeLimit2.AutoSize = true;
            this._theLabelTimeLimit2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelTimeLimit2.Location = new System.Drawing.Point(7, 199);
            this._theLabelTimeLimit2.Name = "_theLabelTimeLimit2";
            this._theLabelTimeLimit2.Size = new System.Drawing.Size(85, 13);
            this._theLabelTimeLimit2.TabIndex = 92;
            this._theLabelTimeLimit2.Text = "Time Limit (secs)";
            // 
            // _theLabelRetryDelay2
            // 
            this._theLabelRetryDelay2.AutoSize = true;
            this._theLabelRetryDelay2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelRetryDelay2.Location = new System.Drawing.Point(336, 147);
            this._theLabelRetryDelay2.Name = "_theLabelRetryDelay2";
            this._theLabelRetryDelay2.Size = new System.Drawing.Size(96, 13);
            this._theLabelRetryDelay2.TabIndex = 90;
            this._theLabelRetryDelay2.Text = "Retry Delay (secs)";
            // 
            // _theLabelRetryCount2
            // 
            this._theLabelRetryCount2.AutoSize = true;
            this._theLabelRetryCount2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelRetryCount2.Location = new System.Drawing.Point(336, 173);
            this._theLabelRetryCount2.Name = "_theLabelRetryCount2";
            this._theLabelRetryCount2.Size = new System.Drawing.Size(66, 13);
            this._theLabelRetryCount2.TabIndex = 88;
            this._theLabelRetryCount2.Text = "Retry Count";
            // 
            // _theLabelLDAPTimeout2
            // 
            this._theLabelLDAPTimeout2.AutoSize = true;
            this._theLabelLDAPTimeout2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelLDAPTimeout2.Location = new System.Drawing.Point(7, 173);
            this._theLabelLDAPTimeout2.Name = "_theLabelLDAPTimeout2";
            this._theLabelLDAPTimeout2.Size = new System.Drawing.Size(105, 13);
            this._theLabelLDAPTimeout2.TabIndex = 87;
            this._theLabelLDAPTimeout2.Text = "LDAP Timeout (secs)";
            // 
            // sectionGroupBox
            // 
            this.sectionGroupBox.Controls.Add(this.TrafodionLabel3);
            this.sectionGroupBox.Controls.Add(this.refreshTimeCheckBox);
            this.sectionGroupBox.Controls.Add(this.remoteCheckBox);
            this.sectionGroupBox.Controls.Add(this.TrafodionLabel1);
            this.sectionGroupBox.Controls.Add(this.refreshTimeNumericUpDown);
            this.sectionGroupBox.Controls.Add(this.localCheckBox);
            this.sectionGroupBox.Controls.Add(this._theLabelDefaultSection);
            this.sectionGroupBox.Controls.Add(this.defaultSectionComboBox);
            this.sectionGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.sectionGroupBox.Location = new System.Drawing.Point(11, 2);
            this.sectionGroupBox.Name = "sectionGroupBox";
            this.sectionGroupBox.Size = new System.Drawing.Size(625, 67);
            this.sectionGroupBox.TabIndex = 42;
            this.sectionGroupBox.TabStop = false;
            this.sectionGroupBox.Text = "Configurations";
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel3.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel3.Location = new System.Drawing.Point(453, 20);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel3.TabIndex = 72;
            this.TrafodionLabel3.Text = "*";
            // 
            // refreshTimeCheckBox
            // 
            this.refreshTimeCheckBox.AutoSize = true;
            this.refreshTimeCheckBox.Checked = true;
            this.refreshTimeCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.refreshTimeCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.refreshTimeCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshTimeCheckBox.Location = new System.Drawing.Point(331, 17);
            this.refreshTimeCheckBox.Name = "refreshTimeCheckBox";
            this.refreshTimeCheckBox.Size = new System.Drawing.Size(127, 18);
            this.refreshTimeCheckBox.TabIndex = 73;
            this.refreshTimeCheckBox.Text = "Refresh Time (mins)";
            this.refreshTimeCheckBox.UseVisualStyleBackColor = true;
            this.refreshTimeCheckBox.Click += new System.EventHandler(this.refreshTimeCheckBox_Click);
            // 
            // remoteCheckBox
            // 
            this.remoteCheckBox.AutoSize = true;
            this.remoteCheckBox.Checked = true;
            this.remoteCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.remoteCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.remoteCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.remoteCheckBox.Location = new System.Drawing.Point(158, 17);
            this.remoteCheckBox.Name = "remoteCheckBox";
            this.remoteCheckBox.Size = new System.Drawing.Size(66, 18);
            this.remoteCheckBox.TabIndex = 1;
            this.remoteCheckBox.Text = "Cluster";
            this.remoteCheckBox.UseVisualStyleBackColor = true;
            this.remoteCheckBox.Click += new System.EventHandler(this.remoteCheckBox_Click);
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel1.Location = new System.Drawing.Point(134, 43);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel1.TabIndex = 71;
            this.TrafodionLabel1.Text = "*";
            // 
            // localCheckBox
            // 
            this.localCheckBox.AutoSize = true;
            this.localCheckBox.Checked = true;
            this.localCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.localCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.localCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.localCheckBox.Location = new System.Drawing.Point(24, 17);
            this.localCheckBox.Name = "localCheckBox";
            this.localCheckBox.Size = new System.Drawing.Size(81, 18);
            this.localCheckBox.TabIndex = 0;
            this.localCheckBox.Text = "Enterprise";
            this.localCheckBox.UseVisualStyleBackColor = true;
            this.localCheckBox.Click += new System.EventHandler(this.localCheckBox_Click);
            // 
            // configurationToolTip
            // 
            this.configurationToolTip.IsBalloon = true;
            // 
            // _theTrafodionPanelBottom
            // 
            this._theTrafodionPanelBottom.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTrafodionPanelBottom.Controls.Add(this.helpButton);
            this._theTrafodionPanelBottom.Controls.Add(this.saveButton);
            this._theTrafodionPanelBottom.Controls.Add(this.cancelButton);
            this._theTrafodionPanelBottom.Controls.Add(this.refreshButton);
            this._theTrafodionPanelBottom.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theTrafodionPanelBottom.Location = new System.Drawing.Point(0, 587);
            this._theTrafodionPanelBottom.Name = "_theTrafodionPanelBottom";
            this._theTrafodionPanelBottom.Size = new System.Drawing.Size(648, 33);
            this._theTrafodionPanelBottom.TabIndex = 43;
            // 
            // _theTrafodionPanelContent
            // 
            this._theTrafodionPanelContent.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTrafodionPanelContent.Controls.Add(this.sectionGroupBox);
            this._theTrafodionPanelContent.Controls.Add(this.sectionTabControl);
            this._theTrafodionPanelContent.Controls.Add(this.defaultsGroupBox);
            this._theTrafodionPanelContent.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTrafodionPanelContent.Location = new System.Drawing.Point(0, 0);
            this._theTrafodionPanelContent.Name = "_theTrafodionPanelContent";
            this._theTrafodionPanelContent.Size = new System.Drawing.Size(648, 587);
            this._theTrafodionPanelContent.TabIndex = 44;
            // 
            // LDAPConnectionConfigDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(648, 642);
            this.Controls.Add(this._theTrafodionPanelContent);
            this.Controls.Add(this._theTrafodionPanelBottom);
            this.Controls.Add(this.statusStrip);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "LDAPConnectionConfigDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Update LDAP Configuration";
            this.defaultsGroupBox.ResumeLayout(false);
            this.defaultsGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.refreshTimeNumericUpDown)).EndInit();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.sectionTabControl.ResumeLayout(false);
            this.tabPageEnterprise.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.attributesGroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.networkTimeoutNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.retryCountNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.ldapTimeoutNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.timelimitNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.retryDelayNumericUpDown)).EndInit();
            this.tabPageCluster.ResumeLayout(false);
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.attributes1GroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.retryCountNumericUpDown1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.retryDelayNumericUpDown1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.ldapTimeoutNumericUpDown1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.networkTimeoutNumericUpDown1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.timelimitNumericUpDown1)).EndInit();
            this.sectionGroupBox.ResumeLayout(false);
            this.sectionGroupBox.PerformLayout();
            this._theTrafodionPanelBottom.ResumeLayout(false);
            this._theTrafodionPanelContent.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox defaultsGroupBox;
        private Framework.Controls.TrafodionButton saveButton;
        private Framework.Controls.TrafodionButton cancelButton;
        private Framework.Controls.TrafodionTextBox cerLocationTextBox;
        private Framework.Controls.TrafodionLabel _theLabelServerCertFile;
        private Framework.Controls.TrafodionButton helpButton;
        private Framework.Controls.TrafodionButton refreshButton;
        private Framework.Controls.TrafodionRichTextBox certificateRichTextBox;
        private Framework.Controls.TrafodionLabel _theLabelCertContent;
        private System.ComponentModel.BackgroundWorker backgroundWorker;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripProgressBar toolStripProgressBar1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private Framework.Controls.TrafodionLabel _theLabelDefaultSection;
        private Framework.Controls.TrafodionComboBox defaultSectionComboBox;
        private Framework.Controls.TrafodionNumericUpDown refreshTimeNumericUpDown;
        private Framework.Controls.TrafodionTabControl sectionTabControl;
        private System.Windows.Forms.TabPage tabPageEnterprise;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionLabel _theLabelLDAPHostName;
        private Framework.Controls.TrafodionComboBox connectionComboBox;
        private Framework.Controls.TrafodionLabel _theLabelNetTimeout;
        private Framework.Controls.TrafodionLabel _theLabelPReserveConn;
        private Framework.Controls.TrafodionLabel _theLabelLDAPTimeout;
        private Framework.Controls.TrafodionTextBox hostNameTextBox;
        private Framework.Controls.TrafodionNumericUpDown networkTimeoutNumericUpDown;
        private Framework.Controls.TrafodionComboBox encryptTypeComboBox;
        private Framework.Controls.TrafodionNumericUpDown retryCountNumericUpDown;
        private Framework.Controls.TrafodionLabel _theLabelEncryptType;
        private Framework.Controls.TrafodionLabel _theLabelRetryCount;
        private Framework.Controls.TrafodionLabel _theLabelConfirmPWD;
        private Framework.Controls.TrafodionNumericUpDown ldapTimeoutNumericUpDown;
        private Framework.Controls.TrafodionNumericUpDown timelimitNumericUpDown;
        private Framework.Controls.TrafodionLabel _theLabelRetryDelay;
        private Framework.Controls.TrafodionTextBox searchDNTextBox;
        private Framework.Controls.TrafodionTextBox confirmPasswdMaskedTextBox;
        private Framework.Controls.TrafodionLabel _theLabelSearchDN;
        private Framework.Controls.TrafodionLabel _theLabelPwd;
        private Framework.Controls.TrafodionNumericUpDown retryDelayNumericUpDown;
        private Framework.Controls.TrafodionLabel _theLabelTimeLimit;
        private Framework.Controls.TrafodionLabel _theLabelLDAPPort;
        private Framework.Controls.TrafodionTextBox passwdMaskedTextBox;
        private System.Windows.Forms.TabPage tabPageCluster;
        private Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Framework.Controls.TrafodionTextBox searchDNTextBox1;
        private Framework.Controls.TrafodionLabel _theLabelSearchDN2;
        private Framework.Controls.TrafodionTextBox confirmPasswdMaskedTextBox1;
        private Framework.Controls.TrafodionTextBox passwdMaskedTextBox1;
        private Framework.Controls.TrafodionLabel _theLabelConfirmPWD2;
        private Framework.Controls.TrafodionLabel _theLabelPWD2;
        private Framework.Controls.TrafodionComboBox encryptTypeComboBox1;
        private Framework.Controls.TrafodionLabel _theLabelEncryptionType;
        private Framework.Controls.TrafodionComboBox connectionComboBox1;
        private Framework.Controls.TrafodionLabel _theLabelPreserveConn2;
        private Framework.Controls.TrafodionTextBox hostNameTextBox1;
        private Framework.Controls.TrafodionLabel _theLabelLDAPHostName2;
        private Framework.Controls.TrafodionGroupBox attributes1GroupBox;
        private ConfigAttributeSettingControl configAttributeSettingControl1;
        private Framework.Controls.TrafodionNumericUpDown retryCountNumericUpDown1;
        private Framework.Controls.TrafodionNumericUpDown retryDelayNumericUpDown1;
        private Framework.Controls.TrafodionNumericUpDown ldapTimeoutNumericUpDown1;
        private Framework.Controls.TrafodionNumericUpDown networkTimeoutNumericUpDown1;
        private Framework.Controls.TrafodionNumericUpDown timelimitNumericUpDown1;
        private Framework.Controls.TrafodionLabel _theLabelLDAPPort2;
        private Framework.Controls.TrafodionLabel _theLabelNetTimeout2;
        private Framework.Controls.TrafodionLabel _theLabelTimeLimit2;
        private Framework.Controls.TrafodionLabel _theLabelRetryDelay2;
        private Framework.Controls.TrafodionLabel _theLabelRetryCount2;
        private Framework.Controls.TrafodionLabel _theLabelLDAPTimeout2;
        private Framework.Controls.TrafodionButton browseButton;
        private Framework.Controls.TrafodionGroupBox sectionGroupBox;
        private Framework.Controls.TrafodionCheckBox remoteCheckBox;
        private Framework.Controls.TrafodionCheckBox localCheckBox;
        private Framework.Controls.TrafodionTextBox portTextBox;
        private Framework.Controls.TrafodionTextBox portTextBox1;
        private Framework.Controls.TrafodionToolTip configurationToolTip;
        private Framework.Controls.TrafodionGroupBox attributesGroupBox;
        private ConfigAttributeSettingControl configAttributeSettingControl;
        private Framework.Controls.TrafodionPanel _theTrafodionPanelBottom;
        private Framework.Controls.TrafodionPanel _theTrafodionPanelContent;
        private Framework.Controls.TrafodionLabel TrafodionLabel7;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Framework.Controls.TrafodionLabel TrafodionLabel4;
        private Framework.Controls.TrafodionLabel TrafodionLabel5;
        private Framework.Controls.TrafodionLabel TrafodionLabel6;
        private Framework.Controls.TrafodionLabel TrafodionLabel8;
        private Framework.Controls.TrafodionCheckBox refreshTimeCheckBox;
        private Framework.Controls.TrafodionButton loadServerCerButton;
    }
}
