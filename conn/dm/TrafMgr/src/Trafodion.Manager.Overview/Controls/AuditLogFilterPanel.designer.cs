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
    partial class AuditLogFilterPanel
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
            this._theTopPanel = new System.Windows.Forms.TableLayoutPanel();
            this._theMessageGroupBox = new System.Windows.Forms.GroupBox();
            this.pnlMessageText = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theMessageFilterConditionCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.lblFilterCondition = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblMessage = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theAuditLogMessageText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theInCaseSensitiveCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theAuditLoggingTimeGroupBox = new System.Windows.Forms.GroupBox();
            this.panelCustomRange = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.lblCustomRange = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theEndTime = new System.Windows.Forms.DateTimePicker();
            this._theCurrentTimeCheckbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theStartTime = new System.Windows.Forms.DateTimePicker();
            this.lblStartTime = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblEndTime = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.panelTimeRange = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.lblTimeRange = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theTimeRangeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theTransactionIDGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblTransactionID = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theTransactionIDText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theOutcomeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theOutcomeCheckListBox = new System.Windows.Forms.CheckedListBox();
            this._theSQLCodeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblSQLCode = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSQLCodeText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theSessionIDGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblSessionID = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSessionIDText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theUserGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblUser = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theInternalUserNameText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theExternalUserNameText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.lblUserInternalName = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblUserExternalName = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theAuditTypeGroupBox = new System.Windows.Forms.GroupBox();
            this._theAuditTypeCheckedListBox = new System.Windows.Forms.CheckedListBox();
            this.pnlSelectAllAuditType = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSelectAllAuditType = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theTopPanel.SuspendLayout();
            this._theMessageGroupBox.SuspendLayout();
            this.pnlMessageText.SuspendLayout();
            this._theAuditLoggingTimeGroupBox.SuspendLayout();
            this.panelCustomRange.SuspendLayout();
            this.panelTimeRange.SuspendLayout();
            this._theTransactionIDGroupBox.SuspendLayout();
            this._theOutcomeGroupBox.SuspendLayout();
            this._theSQLCodeGroupBox.SuspendLayout();
            this._theSessionIDGroupBox.SuspendLayout();
            this._theUserGroupBox.SuspendLayout();
            this._theAuditTypeGroupBox.SuspendLayout();
            this.pnlSelectAllAuditType.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theTopPanel
            // 
            this._theTopPanel.AutoScroll = true;
            this._theTopPanel.AutoScrollMargin = new System.Drawing.Size(0, 200);
            this._theTopPanel.AutoSize = true;
            this._theTopPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theTopPanel.ColumnCount = 1;
            this._theTopPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this._theTopPanel.Controls.Add(this._theMessageGroupBox, 0, 7);
            this._theTopPanel.Controls.Add(this._theAuditLoggingTimeGroupBox, 0, 1);
            this._theTopPanel.Controls.Add(this._theTransactionIDGroupBox, 0, 4);
            this._theTopPanel.Controls.Add(this._theOutcomeGroupBox, 0, 5);
            this._theTopPanel.Controls.Add(this._theSQLCodeGroupBox, 0, 6);
            this._theTopPanel.Controls.Add(this._theSessionIDGroupBox, 0, 3);
            this._theTopPanel.Controls.Add(this._theUserGroupBox, 0, 2);
            this._theTopPanel.Controls.Add(this._theAuditTypeGroupBox, 0, 0);
            this._theTopPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTopPanel.Location = new System.Drawing.Point(0, 0);
            this._theTopPanel.Name = "_theTopPanel";
            this._theTopPanel.RowCount = 8;
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 144F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 152F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 128F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 103F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 80F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 90F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 80F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 120F));
            this._theTopPanel.Size = new System.Drawing.Size(365, 909);
            this._theTopPanel.TabIndex = 7;
            // 
            // _theMessageGroupBox
            // 
            this._theMessageGroupBox.Controls.Add(this.pnlMessageText);
            this._theMessageGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMessageGroupBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theMessageGroupBox.Location = new System.Drawing.Point(3, 780);
            this._theMessageGroupBox.Name = "_theMessageGroupBox";
            this._theMessageGroupBox.Size = new System.Drawing.Size(359, 126);
            this._theMessageGroupBox.TabIndex = 12;
            this._theMessageGroupBox.TabStop = false;
            this._theMessageGroupBox.Text = "Message";
            // 
            // pnlMessageText
            // 
            this.pnlMessageText.BackColor = System.Drawing.Color.WhiteSmoke;
            this.pnlMessageText.Controls.Add(this._theMessageFilterConditionCombo);
            this.pnlMessageText.Controls.Add(this.lblFilterCondition);
            this.pnlMessageText.Controls.Add(this.lblMessage);
            this.pnlMessageText.Controls.Add(this._theAuditLogMessageText);
            this.pnlMessageText.Controls.Add(this._theInCaseSensitiveCheckBox);
            this.pnlMessageText.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnlMessageText.Location = new System.Drawing.Point(3, 16);
            this.pnlMessageText.Name = "pnlMessageText";
            this.pnlMessageText.Size = new System.Drawing.Size(353, 107);
            this.pnlMessageText.TabIndex = 3;
            // 
            // _theMessageFilterConditionCombo
            // 
            this._theMessageFilterConditionCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theMessageFilterConditionCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theMessageFilterConditionCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theMessageFilterConditionCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theMessageFilterConditionCombo.FormattingEnabled = true;
            this._theMessageFilterConditionCombo.Items.AddRange(new object[] {
            "Contains",
            "Does Not Contain"});
            this._theMessageFilterConditionCombo.Location = new System.Drawing.Point(90, 68);
            this._theMessageFilterConditionCombo.Name = "_theMessageFilterConditionCombo";
            this._theMessageFilterConditionCombo.Size = new System.Drawing.Size(148, 21);
            this._theMessageFilterConditionCombo.TabIndex = 7;
            // 
            // lblFilterCondition
            // 
            this.lblFilterCondition.AutoSize = true;
            this.lblFilterCondition.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblFilterCondition.Location = new System.Drawing.Point(6, 72);
            this.lblFilterCondition.Name = "lblFilterCondition";
            this.lblFilterCondition.Size = new System.Drawing.Size(82, 13);
            this.lblFilterCondition.TabIndex = 6;
            this.lblFilterCondition.Text = "Filter  Condition";
            // 
            // lblMessage
            // 
            this.lblMessage.AutoSize = true;
            this.lblMessage.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblMessage.Location = new System.Drawing.Point(2, 7);
            this.lblMessage.Name = "lblMessage";
            this.lblMessage.Size = new System.Drawing.Size(322, 13);
            this.lblMessage.TabIndex = 4;
            this.lblMessage.Text = "Type some text that you want to find within the message column:";
            // 
            // _theAuditLogMessageText
            // 
            this._theAuditLogMessageText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theAuditLogMessageText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAuditLogMessageText.Location = new System.Drawing.Point(4, 25);
            this._theAuditLogMessageText.Name = "_theAuditLogMessageText";
            this._theAuditLogMessageText.Size = new System.Drawing.Size(326, 21);
            this._theAuditLogMessageText.TabIndex = 3;
            // 
            // _theInCaseSensitiveCheckBox
            // 
            this._theInCaseSensitiveCheckBox.AutoSize = true;
            this._theInCaseSensitiveCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theInCaseSensitiveCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theInCaseSensitiveCheckBox.Location = new System.Drawing.Point(4, 47);
            this._theInCaseSensitiveCheckBox.Name = "_theInCaseSensitiveCheckBox";
            this._theInCaseSensitiveCheckBox.Size = new System.Drawing.Size(91, 18);
            this._theInCaseSensitiveCheckBox.TabIndex = 5;
            this._theInCaseSensitiveCheckBox.Text = "Ignore Case";
            this._theInCaseSensitiveCheckBox.UseVisualStyleBackColor = true;
            // 
            // _theAuditLoggingTimeGroupBox
            // 
            this._theAuditLoggingTimeGroupBox.Controls.Add(this.panelCustomRange);
            this._theAuditLoggingTimeGroupBox.Controls.Add(this.panelTimeRange);
            this._theAuditLoggingTimeGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAuditLoggingTimeGroupBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theAuditLoggingTimeGroupBox.Location = new System.Drawing.Point(3, 147);
            this._theAuditLoggingTimeGroupBox.Name = "_theAuditLoggingTimeGroupBox";
            this._theAuditLoggingTimeGroupBox.Size = new System.Drawing.Size(359, 146);
            this._theAuditLoggingTimeGroupBox.TabIndex = 3;
            this._theAuditLoggingTimeGroupBox.TabStop = false;
            this._theAuditLoggingTimeGroupBox.Text = "Audit Logging Time (Server Local Time in 24-Hour Format)";
            // 
            // panelCustomRange
            // 
            this.panelCustomRange.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panelCustomRange.Controls.Add(this.lblCustomRange);
            this.panelCustomRange.Controls.Add(this._theEndTime);
            this.panelCustomRange.Controls.Add(this._theCurrentTimeCheckbox);
            this.panelCustomRange.Controls.Add(this._theStartTime);
            this.panelCustomRange.Controls.Add(this.lblStartTime);
            this.panelCustomRange.Controls.Add(this.lblEndTime);
            this.panelCustomRange.Location = new System.Drawing.Point(3, 63);
            this.panelCustomRange.Name = "panelCustomRange";
            this.panelCustomRange.Size = new System.Drawing.Size(353, 74);
            this.panelCustomRange.TabIndex = 10;
            // 
            // lblCustomRange
            // 
            this.lblCustomRange.AutoSize = true;
            this.lblCustomRange.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblCustomRange.Location = new System.Drawing.Point(4, 2);
            this.lblCustomRange.Name = "lblCustomRange";
            this.lblCustomRange.Size = new System.Drawing.Size(134, 13);
            this.lblCustomRange.TabIndex = 10;
            this.lblCustomRange.Text = "OR  Select a custom range";
            // 
            // _theEndTime
            // 
            this._theEndTime.CustomFormat = "yyyy-MM-dd HH:mm:ss";
            this._theEndTime.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theEndTime.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this._theEndTime.Location = new System.Drawing.Point(84, 48);
            this._theEndTime.Name = "_theEndTime";
            this._theEndTime.Size = new System.Drawing.Size(154, 20);
            this._theEndTime.TabIndex = 5;
            // 
            // _theCurrentTimeCheckbox
            // 
            this._theCurrentTimeCheckbox.AutoSize = true;
            this._theCurrentTimeCheckbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCurrentTimeCheckbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCurrentTimeCheckbox.Location = new System.Drawing.Point(244, 47);
            this._theCurrentTimeCheckbox.Name = "_theCurrentTimeCheckbox";
            this._theCurrentTimeCheckbox.Size = new System.Drawing.Size(94, 18);
            this._theCurrentTimeCheckbox.TabIndex = 9;
            this._theCurrentTimeCheckbox.Text = "Current Time";
            this._theCurrentTimeCheckbox.UseVisualStyleBackColor = true;
            this._theCurrentTimeCheckbox.Click += new System.EventHandler(this._theCurrentTimeCheckbox_CheckedChanged);
            // 
            // _theStartTime
            // 
            this._theStartTime.CustomFormat = "yyyy-MM-dd HH:mm:ss";
            this._theStartTime.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
            this._theStartTime.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this._theStartTime.Location = new System.Drawing.Point(84, 22);
            this._theStartTime.Name = "_theStartTime";
            this._theStartTime.Size = new System.Drawing.Size(154, 20);
            this._theStartTime.TabIndex = 6;
            // 
            // lblStartTime
            // 
            this.lblStartTime.AutoSize = true;
            this.lblStartTime.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblStartTime.Location = new System.Drawing.Point(12, 26);
            this.lblStartTime.Name = "lblStartTime";
            this.lblStartTime.Size = new System.Drawing.Size(56, 13);
            this.lblStartTime.TabIndex = 8;
            this.lblStartTime.Text = "Start Time";
            // 
            // lblEndTime
            // 
            this.lblEndTime.AutoSize = true;
            this.lblEndTime.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblEndTime.Location = new System.Drawing.Point(12, 52);
            this.lblEndTime.Name = "lblEndTime";
            this.lblEndTime.Size = new System.Drawing.Size(50, 13);
            this.lblEndTime.TabIndex = 7;
            this.lblEndTime.Text = "End Time";
            // 
            // panelTimeRange
            // 
            this.panelTimeRange.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panelTimeRange.Controls.Add(this.lblTimeRange);
            this.panelTimeRange.Controls.Add(this._theTimeRangeCombo);
            this.panelTimeRange.Dock = System.Windows.Forms.DockStyle.Top;
            this.panelTimeRange.Location = new System.Drawing.Point(3, 16);
            this.panelTimeRange.Name = "panelTimeRange";
            this.panelTimeRange.Size = new System.Drawing.Size(353, 47);
            this.panelTimeRange.TabIndex = 11;
            // 
            // lblTimeRange
            // 
            this.lblTimeRange.AutoSize = true;
            this.lblTimeRange.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblTimeRange.Location = new System.Drawing.Point(12, 21);
            this.lblTimeRange.Name = "lblTimeRange";
            this.lblTimeRange.Size = new System.Drawing.Size(63, 13);
            this.lblTimeRange.TabIndex = 1;
            this.lblTimeRange.Text = "Time Range";
            // 
            // _theTimeRangeCombo
            // 
            this._theTimeRangeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theTimeRangeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theTimeRangeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theTimeRangeCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theTimeRangeCombo.FormattingEnabled = true;
            this._theTimeRangeCombo.Items.AddRange(new object[] {
            "Last 10 Minutes",
            "Last 20 Minutes",
            "Last 30 Minutes",
            "Last 1 Hour",
            "Today",
            "Last 24 Hours",
            "This Week",
            "Last 7 Days"});
            this._theTimeRangeCombo.Location = new System.Drawing.Point(84, 17);
            this._theTimeRangeCombo.Name = "_theTimeRangeCombo";
            this._theTimeRangeCombo.Size = new System.Drawing.Size(218, 21);
            this._theTimeRangeCombo.TabIndex = 0;
            this._theTimeRangeCombo.SelectedIndexChanged += new System.EventHandler(this._theTimeRangeCombo_SelectedIndexChanged);
            // 
            // _theTransactionIDGroupBox
            // 
            this._theTransactionIDGroupBox.Controls.Add(this.lblTransactionID);
            this._theTransactionIDGroupBox.Controls.Add(this._theTransactionIDText);
            this._theTransactionIDGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTransactionIDGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theTransactionIDGroupBox.Location = new System.Drawing.Point(3, 530);
            this._theTransactionIDGroupBox.Name = "_theTransactionIDGroupBox";
            this._theTransactionIDGroupBox.Size = new System.Drawing.Size(359, 74);
            this._theTransactionIDGroupBox.TabIndex = 8;
            this._theTransactionIDGroupBox.TabStop = false;
            this._theTransactionIDGroupBox.Text = "Transaction ID";
            // 
            // lblTransactionID
            // 
            this.lblTransactionID.AutoSize = true;
            this.lblTransactionID.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblTransactionID.Location = new System.Drawing.Point(10, 17);
            this.lblTransactionID.Name = "lblTransactionID";
            this.lblTransactionID.Size = new System.Drawing.Size(229, 13);
            this.lblTransactionID.TabIndex = 2;
            this.lblTransactionID.Text = "Type Transaction ID(s) separated by commas:";
            // 
            // _theTransactionIDText
            // 
            this._theTransactionIDText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theTransactionIDText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTransactionIDText.Location = new System.Drawing.Point(9, 39);
            this._theTransactionIDText.Name = "_theTransactionIDText";
            this._theTransactionIDText.Size = new System.Drawing.Size(344, 21);
            this._theTransactionIDText.TabIndex = 0;
            // 
            // _theOutcomeGroupBox
            // 
            this._theOutcomeGroupBox.Controls.Add(this._theOutcomeCheckListBox);
            this._theOutcomeGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theOutcomeGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theOutcomeGroupBox.Location = new System.Drawing.Point(3, 610);
            this._theOutcomeGroupBox.Name = "_theOutcomeGroupBox";
            this._theOutcomeGroupBox.Size = new System.Drawing.Size(359, 84);
            this._theOutcomeGroupBox.TabIndex = 9;
            this._theOutcomeGroupBox.TabStop = false;
            this._theOutcomeGroupBox.Text = "Outcome";
            // 
            // _theOutcomeCheckListBox
            // 
            this._theOutcomeCheckListBox.BackColor = System.Drawing.SystemColors.Control;
            this._theOutcomeCheckListBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._theOutcomeCheckListBox.CheckOnClick = true;
            this._theOutcomeCheckListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theOutcomeCheckListBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOutcomeCheckListBox.FormattingEnabled = true;
            this._theOutcomeCheckListBox.Location = new System.Drawing.Point(3, 17);
            this._theOutcomeCheckListBox.Name = "_theOutcomeCheckListBox";
            this._theOutcomeCheckListBox.Size = new System.Drawing.Size(353, 64);
            this._theOutcomeCheckListBox.TabIndex = 0;
            // 
            // _theSQLCodeGroupBox
            // 
            this._theSQLCodeGroupBox.Controls.Add(this.lblSQLCode);
            this._theSQLCodeGroupBox.Controls.Add(this._theSQLCodeText);
            this._theSQLCodeGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSQLCodeGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theSQLCodeGroupBox.Location = new System.Drawing.Point(3, 700);
            this._theSQLCodeGroupBox.Name = "_theSQLCodeGroupBox";
            this._theSQLCodeGroupBox.Size = new System.Drawing.Size(359, 74);
            this._theSQLCodeGroupBox.TabIndex = 10;
            this._theSQLCodeGroupBox.TabStop = false;
            this._theSQLCodeGroupBox.Text = "SQL Code";
            // 
            // lblSQLCode
            // 
            this.lblSQLCode.AutoSize = true;
            this.lblSQLCode.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblSQLCode.Location = new System.Drawing.Point(6, 16);
            this.lblSQLCode.Name = "lblSQLCode";
            this.lblSQLCode.Size = new System.Drawing.Size(193, 13);
            this.lblSQLCode.TabIndex = 2;
            this.lblSQLCode.Text = "Type SQL Code separated by commas:";
            // 
            // _theSQLCodeText
            // 
            this._theSQLCodeText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSQLCodeText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSQLCodeText.Location = new System.Drawing.Point(6, 34);
            this._theSQLCodeText.Name = "_theSQLCodeText";
            this._theSQLCodeText.Size = new System.Drawing.Size(344, 21);
            this._theSQLCodeText.TabIndex = 0;
            // 
            // _theSessionIDGroupBox
            // 
            this._theSessionIDGroupBox.Controls.Add(this.lblSessionID);
            this._theSessionIDGroupBox.Controls.Add(this._theSessionIDText);
            this._theSessionIDGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSessionIDGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theSessionIDGroupBox.Location = new System.Drawing.Point(3, 427);
            this._theSessionIDGroupBox.Name = "_theSessionIDGroupBox";
            this._theSessionIDGroupBox.Size = new System.Drawing.Size(359, 97);
            this._theSessionIDGroupBox.TabIndex = 8;
            this._theSessionIDGroupBox.TabStop = false;
            this._theSessionIDGroupBox.Text = "Session ID";
            // 
            // lblSessionID
            // 
            this.lblSessionID.AutoSize = true;
            this.lblSessionID.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblSessionID.Location = new System.Drawing.Point(10, 17);
            this.lblSessionID.Name = "lblSessionID";
            this.lblSessionID.Size = new System.Drawing.Size(209, 13);
            this.lblSessionID.TabIndex = 2;
            this.lblSessionID.Text = "Type Session ID(s) separated by commas:";
            // 
            // _theSessionIDText
            // 
            this._theSessionIDText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSessionIDText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSessionIDText.Location = new System.Drawing.Point(9, 39);
            this._theSessionIDText.Multiline = true;
            this._theSessionIDText.Name = "_theSessionIDText";
            this._theSessionIDText.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this._theSessionIDText.Size = new System.Drawing.Size(344, 49);
            this._theSessionIDText.TabIndex = 0;
            // 
            // _theUserGroupBox
            // 
            this._theUserGroupBox.Controls.Add(this.lblUser);
            this._theUserGroupBox.Controls.Add(this._theInternalUserNameText);
            this._theUserGroupBox.Controls.Add(this._theExternalUserNameText);
            this._theUserGroupBox.Controls.Add(this.lblUserInternalName);
            this._theUserGroupBox.Controls.Add(this.lblUserExternalName);
            this._theUserGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theUserGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this._theUserGroupBox.Location = new System.Drawing.Point(3, 299);
            this._theUserGroupBox.Name = "_theUserGroupBox";
            this._theUserGroupBox.Size = new System.Drawing.Size(359, 122);
            this._theUserGroupBox.TabIndex = 5;
            this._theUserGroupBox.TabStop = false;
            this._theUserGroupBox.Text = "User";
            // 
            // lblUser
            // 
            this.lblUser.AutoSize = true;
            this.lblUser.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblUser.Location = new System.Drawing.Point(3, 13);
            this.lblUser.Name = "lblUser";
            this.lblUser.Size = new System.Drawing.Size(211, 13);
            this.lblUser.TabIndex = 1;
            this.lblUser.Text = "Type User Name(s) separated by commas:";
            // 
            // _theInternalUserNameText
            // 
            this._theInternalUserNameText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theInternalUserNameText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theInternalUserNameText.Location = new System.Drawing.Point(6, 92);
            this._theInternalUserNameText.Name = "_theInternalUserNameText";
            this._theInternalUserNameText.Size = new System.Drawing.Size(339, 21);
            this._theInternalUserNameText.TabIndex = 0;
            // 
            // _theExternalUserNameText
            // 
            this._theExternalUserNameText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theExternalUserNameText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theExternalUserNameText.Location = new System.Drawing.Point(6, 51);
            this._theExternalUserNameText.Name = "_theExternalUserNameText";
            this._theExternalUserNameText.Size = new System.Drawing.Size(339, 21);
            this._theExternalUserNameText.TabIndex = 0;
            // 
            // lblUserInternalName
            // 
            this.lblUserInternalName.AutoSize = true;
            this.lblUserInternalName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblUserInternalName.Location = new System.Drawing.Point(10, 75);
            this.lblUserInternalName.Name = "lblUserInternalName";
            this.lblUserInternalName.Size = new System.Drawing.Size(125, 13);
            this.lblUserInternalName.TabIndex = 2;
            this.lblUserInternalName.Text = "Database User Name(s):";
            // 
            // lblUserExternalName
            // 
            this.lblUserExternalName.AutoSize = true;
            this.lblUserExternalName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblUserExternalName.Location = new System.Drawing.Point(10, 34);
            this.lblUserExternalName.Name = "lblUserExternalName";
            this.lblUserExternalName.Size = new System.Drawing.Size(162, 13);
            this.lblUserExternalName.TabIndex = 2;
            this.lblUserExternalName.Text = "Directory-Service User Name(s):";
            // 
            // _theAuditTypeGroupBox
            // 
            this._theAuditTypeGroupBox.Controls.Add(this._theAuditTypeCheckedListBox);
            this._theAuditTypeGroupBox.Controls.Add(this.pnlSelectAllAuditType);
            this._theAuditTypeGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAuditTypeGroupBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theAuditTypeGroupBox.Location = new System.Drawing.Point(3, 3);
            this._theAuditTypeGroupBox.Name = "_theAuditTypeGroupBox";
            this._theAuditTypeGroupBox.Size = new System.Drawing.Size(359, 138);
            this._theAuditTypeGroupBox.TabIndex = 13;
            this._theAuditTypeGroupBox.TabStop = false;
            this._theAuditTypeGroupBox.Text = "Audit Type";
            // 
            // _theAuditTypeCheckedListBox
            // 
            this._theAuditTypeCheckedListBox.CheckOnClick = true;
            this._theAuditTypeCheckedListBox.ColumnWidth = 139;
            this._theAuditTypeCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAuditTypeCheckedListBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theAuditTypeCheckedListBox.FormattingEnabled = true;
            this._theAuditTypeCheckedListBox.Location = new System.Drawing.Point(3, 16);
            this._theAuditTypeCheckedListBox.MultiColumn = true;
            this._theAuditTypeCheckedListBox.Name = "_theAuditTypeCheckedListBox";
            this._theAuditTypeCheckedListBox.Size = new System.Drawing.Size(353, 89);
            this._theAuditTypeCheckedListBox.TabIndex = 3;
            // 
            // pnlSelectAllAuditType
            // 
            this.pnlSelectAllAuditType.BackColor = System.Drawing.Color.WhiteSmoke;
            this.pnlSelectAllAuditType.Controls.Add(this._theSelectAllAuditType);
            this.pnlSelectAllAuditType.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.pnlSelectAllAuditType.Location = new System.Drawing.Point(3, 105);
            this.pnlSelectAllAuditType.Name = "pnlSelectAllAuditType";
            this.pnlSelectAllAuditType.Size = new System.Drawing.Size(353, 30);
            this.pnlSelectAllAuditType.TabIndex = 2;
            // 
            // _theSelectAllAuditType
            // 
            this._theSelectAllAuditType.AutoSize = true;
            this._theSelectAllAuditType.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSelectAllAuditType.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSelectAllAuditType.Location = new System.Drawing.Point(4, 6);
            this._theSelectAllAuditType.Name = "_theSelectAllAuditType";
            this._theSelectAllAuditType.Size = new System.Drawing.Size(75, 18);
            this._theSelectAllAuditType.TabIndex = 0;
            this._theSelectAllAuditType.Text = "Select All";
            this._theSelectAllAuditType.UseVisualStyleBackColor = true;
            this._theSelectAllAuditType.CheckedChanged += new System.EventHandler(this._theSelectAllAuditType_CheckedChanged);
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // AuditLogFilterPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theTopPanel);
            this.Name = "AuditLogFilterPanel";
            this.Size = new System.Drawing.Size(365, 909);
            this.SizeChanged += new System.EventHandler(this.AuditLogFilterPanel_SizeChanged);
            this._theTopPanel.ResumeLayout(false);
            this._theMessageGroupBox.ResumeLayout(false);
            this.pnlMessageText.ResumeLayout(false);
            this.pnlMessageText.PerformLayout();
            this._theAuditLoggingTimeGroupBox.ResumeLayout(false);
            this.panelCustomRange.ResumeLayout(false);
            this.panelCustomRange.PerformLayout();
            this.panelTimeRange.ResumeLayout(false);
            this.panelTimeRange.PerformLayout();
            this._theTransactionIDGroupBox.ResumeLayout(false);
            this._theTransactionIDGroupBox.PerformLayout();
            this._theOutcomeGroupBox.ResumeLayout(false);
            this._theSQLCodeGroupBox.ResumeLayout(false);
            this._theSQLCodeGroupBox.PerformLayout();
            this._theSessionIDGroupBox.ResumeLayout(false);
            this._theSessionIDGroupBox.PerformLayout();
            this._theUserGroupBox.ResumeLayout(false);
            this._theUserGroupBox.PerformLayout();
            this._theAuditTypeGroupBox.ResumeLayout(false);
            this.pnlSelectAllAuditType.ResumeLayout(false);
            this.pnlSelectAllAuditType.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel _theTopPanel;
        private Framework.Controls.TrafodionGroupBox _theUserGroupBox;
        private Framework.Controls.TrafodionLabel lblUser;
        private Framework.Controls.TrafodionToolTip _theToolTip;
        private Framework.Controls.TrafodionTextBox _theInternalUserNameText;
        private Framework.Controls.TrafodionTextBox _theExternalUserNameText;
        private Framework.Controls.TrafodionLabel lblUserInternalName;
        private Framework.Controls.TrafodionLabel lblUserExternalName;
        private System.Windows.Forms.GroupBox _theAuditLoggingTimeGroupBox;
        private Framework.Controls.TrafodionPanel panelCustomRange;
        private Framework.Controls.TrafodionLabel lblCustomRange;
        private System.Windows.Forms.DateTimePicker _theEndTime;
        private Framework.Controls.TrafodionCheckBox _theCurrentTimeCheckbox;
        private System.Windows.Forms.DateTimePicker _theStartTime;
        private Framework.Controls.TrafodionLabel lblStartTime;
        private Framework.Controls.TrafodionLabel lblEndTime;
        private Framework.Controls.TrafodionPanel panelTimeRange;
        private Framework.Controls.TrafodionLabel lblTimeRange;
        private Framework.Controls.TrafodionComboBox _theTimeRangeCombo;
        private Framework.Controls.TrafodionGroupBox _theTransactionIDGroupBox;
        private Framework.Controls.TrafodionTextBox _theTransactionIDText;
        private Framework.Controls.TrafodionLabel lblTransactionID;
        private Framework.Controls.TrafodionGroupBox _theOutcomeGroupBox;
        private Framework.Controls.TrafodionGroupBox _theSQLCodeGroupBox;
        private Framework.Controls.TrafodionLabel lblSQLCode;
        private Framework.Controls.TrafodionTextBox _theSQLCodeText;
        private Framework.Controls.TrafodionGroupBox _theSessionIDGroupBox;
        private Framework.Controls.TrafodionLabel lblSessionID;
        private Framework.Controls.TrafodionTextBox _theSessionIDText;
        private System.Windows.Forms.GroupBox _theMessageGroupBox;
        private Framework.Controls.TrafodionPanel pnlMessageText;
        private Framework.Controls.TrafodionComboBox _theMessageFilterConditionCombo;
        private Framework.Controls.TrafodionLabel lblFilterCondition;
        private Framework.Controls.TrafodionLabel lblMessage;
        private Framework.Controls.TrafodionTextBox _theAuditLogMessageText;
        private Framework.Controls.TrafodionCheckBox _theInCaseSensitiveCheckBox;
        private System.Windows.Forms.GroupBox _theAuditTypeGroupBox;
        private Framework.Controls.TrafodionPanel pnlSelectAllAuditType;
        private Framework.Controls.TrafodionCheckBox _theSelectAllAuditType;
        private System.Windows.Forms.CheckedListBox _theOutcomeCheckListBox;
        private System.Windows.Forms.CheckedListBox _theAuditTypeCheckedListBox;
    }
}
