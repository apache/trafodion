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
    partial class AuditLoggingConfiguration
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this._theCloseTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOKTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theRestoreTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theHelpTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theClearTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._statusStrip = new System.Windows.Forms.StatusStrip();
            this._progressBar = new System.Windows.Forms.ToolStripProgressBar();
            this._statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theResetTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._progressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theLogTypeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theLabelLogType = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLogTypePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theToolTip = new System.Windows.Forms.ToolTip(this.components);
            this._theAuditLogsConfigPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAuditLogAlternateGroup = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theNotLogToAlternateRadiobox = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theLogToAlternateRadiobox = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theEnableChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckGroupBox();
            this._theAuditLogsOptionPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAuthorViolationsLogFailActionDropdown = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theAuthorViolationsChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._thePrivilegeChangesChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theObjectChangesLogFailActionDropdown = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theAuditLogsConfigChangesChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theObjectChangesChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theUserMgmtChangesLogFailActionDropdown = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theUserMgmtChangesChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._thePrivilegeChangesFailuresActionDropdown = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theAuthFailuresActionDropdown = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theAuthSucessChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theAuthFailuresChkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theAuditLogsConfigChangesLogFailActionDropdown = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theAuthSucessFailActionDropdown = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theAuditLogsFailActionLabelPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theLoggingFailActionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theAuditLogThresholdsGroup = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theTableAgingNumUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this._theRefreshConfigNumUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.TrafodionLabel7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblTableAgingReminder = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._statusStrip.SuspendLayout();
            this._buttonsPanel.SuspendLayout();
            this._theLogTypePanel.SuspendLayout();
            this._theAuditLogsConfigPanel.SuspendLayout();
            this._theAuditLogAlternateGroup.SuspendLayout();
            this._theEnableChkbox.SuspendLayout();
            this._theAuditLogsOptionPanel.SuspendLayout();
            this._theAuditLogsFailActionLabelPanel.SuspendLayout();
            this._theAuditLogThresholdsGroup.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theTableAgingNumUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this._theRefreshConfigNumUpDown)).BeginInit();
            this.SuspendLayout();
            // 
            // _theCloseTrafodionButton
            // 
            this._theCloseTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCloseTrafodionButton.DialogResult = System.Windows.Forms.DialogResult.No;
            this._theCloseTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCloseTrafodionButton.Location = new System.Drawing.Point(609, 9);
            this._theCloseTrafodionButton.Name = "_theCloseTrafodionButton";
            this._theCloseTrafodionButton.Size = new System.Drawing.Size(75, 23);
            this._theCloseTrafodionButton.TabIndex = 6;
            this._theCloseTrafodionButton.Text = "&Close";
            this._theCloseTrafodionButton.UseVisualStyleBackColor = true;
            // 
            // _theOKTrafodionButton
            // 
            this._theOKTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theOKTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOKTrafodionButton.Location = new System.Drawing.Point(447, 9);
            this._theOKTrafodionButton.Name = "_theOKTrafodionButton";
            this._theOKTrafodionButton.Size = new System.Drawing.Size(75, 23);
            this._theOKTrafodionButton.TabIndex = 5;
            this._theOKTrafodionButton.Text = "&Apply";
            this._theOKTrafodionButton.UseVisualStyleBackColor = true;
            this._theOKTrafodionButton.Click += new System.EventHandler(this._theOKTrafodionButton_Click);
            // 
            // _theRestoreTrafodionButton
            // 
            this._theRestoreTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theRestoreTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRestoreTrafodionButton.Location = new System.Drawing.Point(11, 9);
            this._theRestoreTrafodionButton.Name = "_theRestoreTrafodionButton";
            this._theRestoreTrafodionButton.Size = new System.Drawing.Size(78, 23);
            this._theRestoreTrafodionButton.TabIndex = 5;
            this._theRestoreTrafodionButton.Text = "&Restore";
            this._theRestoreTrafodionButton.UseVisualStyleBackColor = true;
            this._theRestoreTrafodionButton.Click += new System.EventHandler(this._theRestoreTrafodionButton_Click);
            // 
            // _theHelpTrafodionButton
            // 
            this._theHelpTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theHelpTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theHelpTrafodionButton.Location = new System.Drawing.Point(689, 9);
            this._theHelpTrafodionButton.Name = "_theHelpTrafodionButton";
            this._theHelpTrafodionButton.Size = new System.Drawing.Size(75, 23);
            this._theHelpTrafodionButton.TabIndex = 6;
            this._theHelpTrafodionButton.Text = "&Help";
            this._theHelpTrafodionButton.UseVisualStyleBackColor = true;
            this._theHelpTrafodionButton.Click += new System.EventHandler(this._theHelpTrafodionButton_Click);
            // 
            // _theClearTrafodionButton
            // 
            this._theClearTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theClearTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theClearTrafodionButton.Location = new System.Drawing.Point(528, 9);
            this._theClearTrafodionButton.Name = "_theClearTrafodionButton";
            this._theClearTrafodionButton.Size = new System.Drawing.Size(75, 23);
            this._theClearTrafodionButton.TabIndex = 5;
            this._theClearTrafodionButton.Text = "C&lear";
            this._theClearTrafodionButton.UseVisualStyleBackColor = true;
            this._theClearTrafodionButton.Click += new System.EventHandler(this._theClearTrafodionButton_Click);
            // 
            // _statusStrip
            // 
            this._statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._progressBar,
            this._statusLabel});
            this._statusStrip.Location = new System.Drawing.Point(0, 579);
            this._statusStrip.Name = "_statusStrip";
            this._statusStrip.Size = new System.Drawing.Size(774, 22);
            this._statusStrip.TabIndex = 13;
            this._statusStrip.Text = "TrafodionStatusStrip1";
            // 
            // _progressBar
            // 
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(150, 16);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // _statusLabel
            // 
            this._statusLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this._statusLabel.Name = "_statusLabel";
            this._statusLabel.Size = new System.Drawing.Size(0, 17);
            this._statusLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._theResetTrafodionButton);
            this._buttonsPanel.Controls.Add(this._theRestoreTrafodionButton);
            this._buttonsPanel.Controls.Add(this._theClearTrafodionButton);
            this._buttonsPanel.Controls.Add(this._theOKTrafodionButton);
            this._buttonsPanel.Controls.Add(this._theCloseTrafodionButton);
            this._buttonsPanel.Controls.Add(this._theHelpTrafodionButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 541);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(774, 38);
            this._buttonsPanel.TabIndex = 17;
            // 
            // _theResetTrafodionButton
            // 
            this._theResetTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theResetTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theResetTrafodionButton.Location = new System.Drawing.Point(97, 9);
            this._theResetTrafodionButton.Name = "_theResetTrafodionButton";
            this._theResetTrafodionButton.Size = new System.Drawing.Size(78, 23);
            this._theResetTrafodionButton.TabIndex = 5;
            this._theResetTrafodionButton.Text = "&Disable All";
            this._theResetTrafodionButton.UseVisualStyleBackColor = true;
            this._theResetTrafodionButton.Click += new System.EventHandler(this._theResetTrafodionButton_Click);
            // 
            // _progressPanel
            // 
            this._progressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._progressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._progressPanel.Location = new System.Drawing.Point(0, 0);
            this._progressPanel.Name = "_progressPanel";
            this._progressPanel.Size = new System.Drawing.Size(774, 29);
            this._progressPanel.TabIndex = 16;
            // 
            // _theLogTypeComboBox
            // 
            this._theLogTypeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theLogTypeComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theLogTypeComboBox.FormattingEnabled = true;
            this._theLogTypeComboBox.Items.AddRange(new object[] {
            "SAR (Security Audit Repository) ",
            "SysLog"});
            this._theLogTypeComboBox.Location = new System.Drawing.Point(78, 6);
            this._theLogTypeComboBox.Name = "_theLogTypeComboBox";
            this._theLogTypeComboBox.Size = new System.Drawing.Size(377, 21);
            this._theLogTypeComboBox.TabIndex = 13;
            this._theLogTypeComboBox.SelectedIndexChanged += new System.EventHandler(this._theLogTypeComboBox_SelectedIndexChanged);
            // 
            // _theLabelLogType
            // 
            this._theLabelLogType.AutoSize = true;
            this._theLabelLogType.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabelLogType.Location = new System.Drawing.Point(17, 9);
            this._theLabelLogType.Name = "_theLabelLogType";
            this._theLabelLogType.Size = new System.Drawing.Size(51, 13);
            this._theLabelLogType.TabIndex = 12;
            this._theLabelLogType.Text = "Log Type";
            // 
            // _theLogTypePanel
            // 
            this._theLogTypePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theLogTypePanel.Controls.Add(this._theLogTypeComboBox);
            this._theLogTypePanel.Controls.Add(this._theLabelLogType);
            this._theLogTypePanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theLogTypePanel.Location = new System.Drawing.Point(0, 29);
            this._theLogTypePanel.Name = "_theLogTypePanel";
            this._theLogTypePanel.Size = new System.Drawing.Size(774, 38);
            this._theLogTypePanel.TabIndex = 21;
            // 
            // _theAuditLogsConfigPanel
            // 
            this._theAuditLogsConfigPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAuditLogsConfigPanel.Controls.Add(this._theAuditLogAlternateGroup);
            this._theAuditLogsConfigPanel.Controls.Add(this._theEnableChkbox);
            this._theAuditLogsConfigPanel.Controls.Add(this._theAuditLogThresholdsGroup);
            this._theAuditLogsConfigPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theAuditLogsConfigPanel.Location = new System.Drawing.Point(0, 67);
            this._theAuditLogsConfigPanel.Name = "_theAuditLogsConfigPanel";
            this._theAuditLogsConfigPanel.Size = new System.Drawing.Size(774, 477);
            this._theAuditLogsConfigPanel.TabIndex = 22;
            // 
            // _theAuditLogAlternateGroup
            // 
            this._theAuditLogAlternateGroup.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theAuditLogAlternateGroup.Controls.Add(this._theNotLogToAlternateRadiobox);
            this._theAuditLogAlternateGroup.Controls.Add(this._theLogToAlternateRadiobox);
            this._theAuditLogAlternateGroup.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theAuditLogAlternateGroup.Location = new System.Drawing.Point(5, 410);
            this._theAuditLogAlternateGroup.Name = "_theAuditLogAlternateGroup";
            this._theAuditLogAlternateGroup.Size = new System.Drawing.Size(766, 63);
            this._theAuditLogAlternateGroup.TabIndex = 14;
            this._theAuditLogAlternateGroup.TabStop = false;
            this._theAuditLogAlternateGroup.Text = "Audit Log Alternate:";
            // 
            // _theNotLogToAlternateRadiobox
            // 
            this._theNotLogToAlternateRadiobox.AutoSize = true;
            this._theNotLogToAlternateRadiobox.Checked = true;
            this._theNotLogToAlternateRadiobox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theNotLogToAlternateRadiobox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theNotLogToAlternateRadiobox.Location = new System.Drawing.Point(19, 20);
            this._theNotLogToAlternateRadiobox.Name = "_theNotLogToAlternateRadiobox";
            this._theNotLogToAlternateRadiobox.Size = new System.Drawing.Size(173, 18);
            this._theNotLogToAlternateRadiobox.TabIndex = 12;
            this._theNotLogToAlternateRadiobox.TabStop = true;
            this._theNotLogToAlternateRadiobox.Text = "Do not log to alternate target";
            this._theNotLogToAlternateRadiobox.UseVisualStyleBackColor = true;
            // 
            // _theLogToAlternateRadiobox
            // 
            this._theLogToAlternateRadiobox.AutoSize = true;
            this._theLogToAlternateRadiobox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theLogToAlternateRadiobox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLogToAlternateRadiobox.Location = new System.Drawing.Point(19, 38);
            this._theLogToAlternateRadiobox.Name = "_theLogToAlternateRadiobox";
            this._theLogToAlternateRadiobox.Size = new System.Drawing.Size(234, 18);
            this._theLogToAlternateRadiobox.TabIndex = 10;
            this._theLogToAlternateRadiobox.Text = "Log event to the security audit repository ";
            this._theLogToAlternateRadiobox.UseVisualStyleBackColor = true;
            // 
            // _theEnableChkbox
            // 
            this._theEnableChkbox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theEnableChkbox.Checked = true;
            this._theEnableChkbox.Controls.Add(this._theAuditLogsOptionPanel);
            this._theEnableChkbox.Controls.Add(this._theAuditLogsFailActionLabelPanel);
            this._theEnableChkbox.EnableDisableInnerControls = true;
            this._theEnableChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theEnableChkbox.Location = new System.Drawing.Point(4, 11);
            this._theEnableChkbox.Name = "_theEnableChkbox";
            this._theEnableChkbox.Size = new System.Drawing.Size(766, 250);
            this._theEnableChkbox.TabIndex = 8;
            this._theEnableChkbox.TabStop = false;
            this._theEnableChkbox.Text = "Enable audit logging";
            this._theEnableChkbox.CheckBoxCheckedChanged += new System.EventHandler(this._theEnableChkbox_CheckBoxCheckedChanged);
            // 
            // _theAuditLogsOptionPanel
            // 
            this._theAuditLogsOptionPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuthorViolationsLogFailActionDropdown);
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuthorViolationsChkbox);
            this._theAuditLogsOptionPanel.Controls.Add(this._thePrivilegeChangesChkbox);
            this._theAuditLogsOptionPanel.Controls.Add(this._theObjectChangesLogFailActionDropdown);
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuditLogsConfigChangesChkbox);
            this._theAuditLogsOptionPanel.Controls.Add(this._theObjectChangesChkbox);
            this._theAuditLogsOptionPanel.Controls.Add(this._theUserMgmtChangesLogFailActionDropdown);
            this._theAuditLogsOptionPanel.Controls.Add(this._theUserMgmtChangesChkbox);
            this._theAuditLogsOptionPanel.Controls.Add(this._thePrivilegeChangesFailuresActionDropdown);
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuthFailuresActionDropdown);
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuthSucessChkbox);
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuthFailuresChkbox);
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuditLogsConfigChangesLogFailActionDropdown);
            this._theAuditLogsOptionPanel.Controls.Add(this._theAuthSucessFailActionDropdown);
            this._theAuditLogsOptionPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theAuditLogsOptionPanel.Location = new System.Drawing.Point(3, 44);
            this._theAuditLogsOptionPanel.Name = "_theAuditLogsOptionPanel";
            this._theAuditLogsOptionPanel.Size = new System.Drawing.Size(760, 200);
            this._theAuditLogsOptionPanel.TabIndex = 17;
            // 
            // _theAuthorViolationsLogFailActionDropdown
            // 
            this._theAuthorViolationsLogFailActionDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theAuthorViolationsLogFailActionDropdown.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuthorViolationsLogFailActionDropdown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuthorViolationsLogFailActionDropdown.FormattingEnabled = true;
            this._theAuthorViolationsLogFailActionDropdown.Location = new System.Drawing.Point(263, 167);
            this._theAuthorViolationsLogFailActionDropdown.Name = "_theAuthorViolationsLogFailActionDropdown";
            this._theAuthorViolationsLogFailActionDropdown.Size = new System.Drawing.Size(192, 21);
            this._theAuthorViolationsLogFailActionDropdown.TabIndex = 26;
            // 
            // _theAuthorViolationsChkbox
            // 
            this._theAuthorViolationsChkbox.AutoSize = true;
            this._theAuthorViolationsChkbox.Checked = true;
            this._theAuthorViolationsChkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._theAuthorViolationsChkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuthorViolationsChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuthorViolationsChkbox.Location = new System.Drawing.Point(24, 166);
            this._theAuthorViolationsChkbox.Name = "_theAuthorViolationsChkbox";
            this._theAuthorViolationsChkbox.Size = new System.Drawing.Size(144, 18);
            this._theAuthorViolationsChkbox.TabIndex = 21;
            this._theAuthorViolationsChkbox.Text = "Authorization violations";
            this._theAuthorViolationsChkbox.UseVisualStyleBackColor = true;
            // 
            // _thePrivilegeChangesChkbox
            // 
            this._thePrivilegeChangesChkbox.AutoSize = true;
            this._thePrivilegeChangesChkbox.Checked = true;
            this._thePrivilegeChangesChkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._thePrivilegeChangesChkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._thePrivilegeChangesChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePrivilegeChangesChkbox.Location = new System.Drawing.Point(24, 87);
            this._thePrivilegeChangesChkbox.Name = "_thePrivilegeChangesChkbox";
            this._thePrivilegeChangesChkbox.Size = new System.Drawing.Size(115, 18);
            this._thePrivilegeChangesChkbox.TabIndex = 18;
            this._thePrivilegeChangesChkbox.Text = "Privilege changes";
            this._thePrivilegeChangesChkbox.UseVisualStyleBackColor = true;
            // 
            // _theObjectChangesLogFailActionDropdown
            // 
            this._theObjectChangesLogFailActionDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theObjectChangesLogFailActionDropdown.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theObjectChangesLogFailActionDropdown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theObjectChangesLogFailActionDropdown.FormattingEnabled = true;
            this._theObjectChangesLogFailActionDropdown.Location = new System.Drawing.Point(263, 140);
            this._theObjectChangesLogFailActionDropdown.Name = "_theObjectChangesLogFailActionDropdown";
            this._theObjectChangesLogFailActionDropdown.Size = new System.Drawing.Size(192, 21);
            this._theObjectChangesLogFailActionDropdown.TabIndex = 25;
            // 
            // _theAuditLogsConfigChangesChkbox
            // 
            this._theAuditLogsConfigChangesChkbox.AutoSize = true;
            this._theAuditLogsConfigChangesChkbox.Checked = true;
            this._theAuditLogsConfigChangesChkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._theAuditLogsConfigChangesChkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuditLogsConfigChangesChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuditLogsConfigChangesChkbox.Location = new System.Drawing.Point(24, 6);
            this._theAuditLogsConfigChangesChkbox.Name = "_theAuditLogsConfigChangesChkbox";
            this._theAuditLogsConfigChangesChkbox.Size = new System.Drawing.Size(221, 18);
            this._theAuditLogsConfigChangesChkbox.TabIndex = 15;
            this._theAuditLogsConfigChangesChkbox.Text = "Audit Repository configuration changes";
            this._theAuditLogsConfigChangesChkbox.UseVisualStyleBackColor = true;
            // 
            // _theObjectChangesChkbox
            // 
            this._theObjectChangesChkbox.AutoSize = true;
            this._theObjectChangesChkbox.Checked = true;
            this._theObjectChangesChkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._theObjectChangesChkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theObjectChangesChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theObjectChangesChkbox.Location = new System.Drawing.Point(24, 141);
            this._theObjectChangesChkbox.Name = "_theObjectChangesChkbox";
            this._theObjectChangesChkbox.Size = new System.Drawing.Size(107, 18);
            this._theObjectChangesChkbox.TabIndex = 20;
            this._theObjectChangesChkbox.Text = "Object changes";
            this._theObjectChangesChkbox.UseVisualStyleBackColor = true;
            // 
            // _theUserMgmtChangesLogFailActionDropdown
            // 
            this._theUserMgmtChangesLogFailActionDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theUserMgmtChangesLogFailActionDropdown.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theUserMgmtChangesLogFailActionDropdown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserMgmtChangesLogFailActionDropdown.FormattingEnabled = true;
            this._theUserMgmtChangesLogFailActionDropdown.Location = new System.Drawing.Point(263, 113);
            this._theUserMgmtChangesLogFailActionDropdown.Name = "_theUserMgmtChangesLogFailActionDropdown";
            this._theUserMgmtChangesLogFailActionDropdown.Size = new System.Drawing.Size(192, 21);
            this._theUserMgmtChangesLogFailActionDropdown.TabIndex = 28;
            // 
            // _theUserMgmtChangesChkbox
            // 
            this._theUserMgmtChangesChkbox.AutoSize = true;
            this._theUserMgmtChangesChkbox.Checked = true;
            this._theUserMgmtChangesChkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._theUserMgmtChangesChkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theUserMgmtChangesChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserMgmtChangesChkbox.Location = new System.Drawing.Point(24, 113);
            this._theUserMgmtChangesChkbox.Name = "_theUserMgmtChangesChkbox";
            this._theUserMgmtChangesChkbox.Size = new System.Drawing.Size(162, 18);
            this._theUserMgmtChangesChkbox.TabIndex = 19;
            this._theUserMgmtChangesChkbox.Text = "User management changes";
            this._theUserMgmtChangesChkbox.UseVisualStyleBackColor = true;
            // 
            // _thePrivilegeChangesFailuresActionDropdown
            // 
            this._thePrivilegeChangesFailuresActionDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._thePrivilegeChangesFailuresActionDropdown.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._thePrivilegeChangesFailuresActionDropdown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePrivilegeChangesFailuresActionDropdown.FormattingEnabled = true;
            this._thePrivilegeChangesFailuresActionDropdown.Location = new System.Drawing.Point(263, 86);
            this._thePrivilegeChangesFailuresActionDropdown.Name = "_thePrivilegeChangesFailuresActionDropdown";
            this._thePrivilegeChangesFailuresActionDropdown.Size = new System.Drawing.Size(192, 21);
            this._thePrivilegeChangesFailuresActionDropdown.TabIndex = 27;
            // 
            // _theAuthFailuresActionDropdown
            // 
            this._theAuthFailuresActionDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theAuthFailuresActionDropdown.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuthFailuresActionDropdown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuthFailuresActionDropdown.FormattingEnabled = true;
            this._theAuthFailuresActionDropdown.Location = new System.Drawing.Point(263, 59);
            this._theAuthFailuresActionDropdown.Name = "_theAuthFailuresActionDropdown";
            this._theAuthFailuresActionDropdown.Size = new System.Drawing.Size(192, 21);
            this._theAuthFailuresActionDropdown.TabIndex = 24;
            // 
            // _theAuthSucessChkbox
            // 
            this._theAuthSucessChkbox.AutoSize = true;
            this._theAuthSucessChkbox.Checked = true;
            this._theAuthSucessChkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._theAuthSucessChkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuthSucessChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuthSucessChkbox.Location = new System.Drawing.Point(24, 33);
            this._theAuthSucessChkbox.Name = "_theAuthSucessChkbox";
            this._theAuthSucessChkbox.Size = new System.Drawing.Size(142, 18);
            this._theAuthSucessChkbox.TabIndex = 16;
            this._theAuthSucessChkbox.Text = "Authentication success";
            this._theAuthSucessChkbox.UseVisualStyleBackColor = true;
            // 
            // _theAuthFailuresChkbox
            // 
            this._theAuthFailuresChkbox.AutoSize = true;
            this._theAuthFailuresChkbox.Checked = true;
            this._theAuthFailuresChkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._theAuthFailuresChkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuthFailuresChkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuthFailuresChkbox.Location = new System.Drawing.Point(24, 60);
            this._theAuthFailuresChkbox.Name = "_theAuthFailuresChkbox";
            this._theAuthFailuresChkbox.Size = new System.Drawing.Size(140, 18);
            this._theAuthFailuresChkbox.TabIndex = 17;
            this._theAuthFailuresChkbox.Text = "Authentication failures";
            this._theAuthFailuresChkbox.UseVisualStyleBackColor = true;
            // 
            // _theAuditLogsConfigChangesLogFailActionDropdown
            // 
            this._theAuditLogsConfigChangesLogFailActionDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theAuditLogsConfigChangesLogFailActionDropdown.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuditLogsConfigChangesLogFailActionDropdown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuditLogsConfigChangesLogFailActionDropdown.FormattingEnabled = true;
            this._theAuditLogsConfigChangesLogFailActionDropdown.Location = new System.Drawing.Point(263, 6);
            this._theAuditLogsConfigChangesLogFailActionDropdown.Name = "_theAuditLogsConfigChangesLogFailActionDropdown";
            this._theAuditLogsConfigChangesLogFailActionDropdown.Size = new System.Drawing.Size(192, 21);
            this._theAuditLogsConfigChangesLogFailActionDropdown.TabIndex = 23;
            // 
            // _theAuthSucessFailActionDropdown
            // 
            this._theAuthSucessFailActionDropdown.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theAuthSucessFailActionDropdown.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAuthSucessFailActionDropdown.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuthSucessFailActionDropdown.FormattingEnabled = true;
            this._theAuthSucessFailActionDropdown.Location = new System.Drawing.Point(263, 32);
            this._theAuthSucessFailActionDropdown.Name = "_theAuthSucessFailActionDropdown";
            this._theAuthSucessFailActionDropdown.Size = new System.Drawing.Size(192, 21);
            this._theAuthSucessFailActionDropdown.TabIndex = 22;
            // 
            // _theAuditLogsFailActionLabelPanel
            // 
            this._theAuditLogsFailActionLabelPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAuditLogsFailActionLabelPanel.Controls.Add(this._theLoggingFailActionLabel);
            this._theAuditLogsFailActionLabelPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theAuditLogsFailActionLabelPanel.Location = new System.Drawing.Point(3, 17);
            this._theAuditLogsFailActionLabelPanel.Name = "_theAuditLogsFailActionLabelPanel";
            this._theAuditLogsFailActionLabelPanel.Size = new System.Drawing.Size(760, 27);
            this._theAuditLogsFailActionLabelPanel.TabIndex = 16;
            // 
            // _theLoggingFailActionLabel
            // 
            this._theLoggingFailActionLabel.AutoSize = true;
            this._theLoggingFailActionLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theLoggingFailActionLabel.Location = new System.Drawing.Point(262, 7);
            this._theLoggingFailActionLabel.Name = "_theLoggingFailActionLabel";
            this._theLoggingFailActionLabel.Size = new System.Drawing.Size(121, 13);
            this._theLoggingFailActionLabel.TabIndex = 15;
            this._theLoggingFailActionLabel.Text = "Logging Fails Action:";
            // 
            // _theAuditLogThresholdsGroup
            // 
            this._theAuditLogThresholdsGroup.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theAuditLogThresholdsGroup.Controls.Add(this.TrafodionLabel2);
            this._theAuditLogThresholdsGroup.Controls.Add(this.TrafodionLabel6);
            this._theAuditLogThresholdsGroup.Controls.Add(this._theTableAgingNumUpDown);
            this._theAuditLogThresholdsGroup.Controls.Add(this._theRefreshConfigNumUpDown);
            this._theAuditLogThresholdsGroup.Controls.Add(this.TrafodionLabel7);
            this._theAuditLogThresholdsGroup.Controls.Add(this.lblTableAgingReminder);
            this._theAuditLogThresholdsGroup.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theAuditLogThresholdsGroup.Location = new System.Drawing.Point(5, 262);
            this._theAuditLogThresholdsGroup.Name = "_theAuditLogThresholdsGroup";
            this._theAuditLogThresholdsGroup.Size = new System.Drawing.Size(766, 145);
            this._theAuditLogThresholdsGroup.TabIndex = 11;
            this._theAuditLogThresholdsGroup.TabStop = false;
            this._theAuditLogThresholdsGroup.Text = "Audit Logging Configuration Thresholds";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(23, 22);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(97, 13);
            this.TrafodionLabel2.TabIndex = 15;
            this.TrafodionLabel2.Text = "Table Aging (days)";
            // 
            // TrafodionLabel6
            // 
            this.TrafodionLabel6.AutoSize = true;
            this.TrafodionLabel6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel6.Location = new System.Drawing.Point(23, 47);
            this.TrafodionLabel6.Name = "TrafodionLabel6";
            this.TrafodionLabel6.Size = new System.Drawing.Size(186, 13);
            this.TrafodionLabel6.TabIndex = 14;
            this.TrafodionLabel6.Text = "Refresh Configuration Time(Seconds)";
            // 
            // _theTableAgingNumUpDown
            // 
            this._theTableAgingNumUpDown.Location = new System.Drawing.Point(258, 18);
            this._theTableAgingNumUpDown.Maximum = new decimal(new int[] {
            366,
            0,
            0,
            0});
            this._theTableAgingNumUpDown.Name = "_theTableAgingNumUpDown";
            this._theTableAgingNumUpDown.Size = new System.Drawing.Size(96, 21);
            this._theTableAgingNumUpDown.TabIndex = 17;
            this._theTableAgingNumUpDown.Value = new decimal(new int[] {
            15,
            0,
            0,
            0});
            // 
            // _theRefreshConfigNumUpDown
            // 
            this._theRefreshConfigNumUpDown.Location = new System.Drawing.Point(258, 43);
            this._theRefreshConfigNumUpDown.Maximum = new decimal(new int[] {
            86400,
            0,
            0,
            0});
            this._theRefreshConfigNumUpDown.Name = "_theRefreshConfigNumUpDown";
            this._theRefreshConfigNumUpDown.Size = new System.Drawing.Size(96, 21);
            this._theRefreshConfigNumUpDown.TabIndex = 16;
            this._theRefreshConfigNumUpDown.Value = new decimal(new int[] {
            210,
            0,
            0,
            0});
            // 
            // TrafodionLabel7
            // 
            this.TrafodionLabel7.AutoSize = true;
            this.TrafodionLabel7.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel7.Location = new System.Drawing.Point(357, 47);
            this.TrafodionLabel7.Name = "TrafodionLabel7";
            this.TrafodionLabel7.Size = new System.Drawing.Size(169, 13);
            this.TrafodionLabel7.TabIndex = 19;
            this.TrafodionLabel7.Text = "(Value: 0 means disable Refresh.)";
            // 
            // lblTableAgingReminder
            // 
            this.lblTableAgingReminder.AutoSize = true;
            this.lblTableAgingReminder.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblTableAgingReminder.Location = new System.Drawing.Point(357, 22);
            this.lblTableAgingReminder.Name = "lblTableAgingReminder";
            this.lblTableAgingReminder.Size = new System.Drawing.Size(203, 13);
            this.lblTableAgingReminder.TabIndex = 18;
            this.lblTableAgingReminder.Text = "(Value: 0 means no default Table Aging.)";
            // 
            // AuditLoggingConfiguration
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(774, 601);
            this.Controls.Add(this._theAuditLogsConfigPanel);
            this.Controls.Add(this._theLogTypePanel);
            this.Controls.Add(this._buttonsPanel);
            this.Controls.Add(this._progressPanel);
            this.Controls.Add(this._statusStrip);
            this.Name = "AuditLoggingConfiguration";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Audit Logging Configuration";
            this._statusStrip.ResumeLayout(false);
            this._statusStrip.PerformLayout();
            this._buttonsPanel.ResumeLayout(false);
            this._theLogTypePanel.ResumeLayout(false);
            this._theLogTypePanel.PerformLayout();
            this._theAuditLogsConfigPanel.ResumeLayout(false);
            this._theAuditLogAlternateGroup.ResumeLayout(false);
            this._theAuditLogAlternateGroup.PerformLayout();
            this._theEnableChkbox.ResumeLayout(false);
            this._theAuditLogsOptionPanel.ResumeLayout(false);
            this._theAuditLogsOptionPanel.PerformLayout();
            this._theAuditLogsFailActionLabelPanel.ResumeLayout(false);
            this._theAuditLogsFailActionLabelPanel.PerformLayout();
            this._theAuditLogThresholdsGroup.ResumeLayout(false);
            this._theAuditLogThresholdsGroup.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theTableAgingNumUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this._theRefreshConfigNumUpDown)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionButton _theCloseTrafodionButton;
        private Framework.Controls.TrafodionButton _theOKTrafodionButton;
        private Framework.Controls.TrafodionButton _theRestoreTrafodionButton;
        private Framework.Controls.TrafodionButton _theHelpTrafodionButton;
        private Framework.Controls.TrafodionButton _theClearTrafodionButton;
        private System.Windows.Forms.StatusStrip _statusStrip;
        private System.Windows.Forms.ToolStripProgressBar _progressBar;
        private System.Windows.Forms.ToolStripStatusLabel _statusLabel;
        private Framework.Controls.TrafodionPanel _buttonsPanel;
        private Framework.Controls.TrafodionPanel _progressPanel;
        private Framework.Controls.TrafodionComboBox _theLogTypeComboBox;
        private Framework.Controls.TrafodionLabel _theLabelLogType;
        private Framework.Controls.TrafodionPanel _theLogTypePanel;
        private System.Windows.Forms.ToolTip _theToolTip;
        private Framework.Controls.TrafodionPanel _theAuditLogsConfigPanel;
        private Framework.Controls.TrafodionGroupBox _theAuditLogAlternateGroup;
        private Framework.Controls.TrafodionRadioButton _theNotLogToAlternateRadiobox;
        private Framework.Controls.TrafodionRadioButton _theLogToAlternateRadiobox;
        private Framework.Controls.TrafodionCheckGroupBox _theEnableChkbox;
        private Framework.Controls.TrafodionPanel _theAuditLogsOptionPanel;
        private Framework.Controls.TrafodionComboBox _theAuthorViolationsLogFailActionDropdown;
        private Framework.Controls.TrafodionCheckBox _theAuthorViolationsChkbox;
        private Framework.Controls.TrafodionCheckBox _thePrivilegeChangesChkbox;
        private Framework.Controls.TrafodionComboBox _theObjectChangesLogFailActionDropdown;
        private Framework.Controls.TrafodionCheckBox _theAuditLogsConfigChangesChkbox;
        private Framework.Controls.TrafodionCheckBox _theObjectChangesChkbox;
        private Framework.Controls.TrafodionComboBox _theUserMgmtChangesLogFailActionDropdown;
        private Framework.Controls.TrafodionCheckBox _theUserMgmtChangesChkbox;
        private Framework.Controls.TrafodionComboBox _thePrivilegeChangesFailuresActionDropdown;
        private Framework.Controls.TrafodionComboBox _theAuthFailuresActionDropdown;
        private Framework.Controls.TrafodionCheckBox _theAuthSucessChkbox;
        private Framework.Controls.TrafodionCheckBox _theAuthFailuresChkbox;
        private Framework.Controls.TrafodionComboBox _theAuditLogsConfigChangesLogFailActionDropdown;
        private Framework.Controls.TrafodionComboBox _theAuthSucessFailActionDropdown;
        private Framework.Controls.TrafodionPanel _theAuditLogsFailActionLabelPanel;
        private Framework.Controls.TrafodionLabel _theLoggingFailActionLabel;
        private Framework.Controls.TrafodionGroupBox _theAuditLogThresholdsGroup;
        private Framework.Controls.TrafodionButton _theResetTrafodionButton;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionLabel TrafodionLabel6;
        private Framework.Controls.TrafodionNumericUpDown _theTableAgingNumUpDown;
        private Framework.Controls.TrafodionNumericUpDown _theRefreshConfigNumUpDown;
        private Framework.Controls.TrafodionLabel TrafodionLabel7;
        private Framework.Controls.TrafodionLabel lblTableAgingReminder;
    }
}