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
namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class AMQPEventsFilterPanel
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
            this._theSubSystemGroupBox = new System.Windows.Forms.GroupBox();
            this._theSubsystemsListbox = new System.Windows.Forms.CheckedListBox();
            this.TrafodionPanel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSubSystemSelectAllCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theSeverityGroupBox = new System.Windows.Forms.GroupBox();
            this._theSeverityListBox = new System.Windows.Forms.CheckedListBox();
            this.TrafodionPanel4 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSeveritySelectAllCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theEventIdGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theEventIdText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theProcessIdGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionLabel10 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theProcessNameText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel9 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theProcessIdText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theMessageGroupBox = new System.Windows.Forms.GroupBox();
            this.TrafodionPanel7 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theMessageFilterConditionCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionLabel7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel6 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theInCaseSensitiveCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theEventText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theEventTimeGroupBox = new System.Windows.Forms.GroupBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theEndTime = new System.Windows.Forms.DateTimePicker();
            this._theCurrentTimeCheckbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theStartTime = new System.Windows.Forms.DateTimePicker();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theTimeRangeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theFilterButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theResestButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theApplyFilterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theTopPanel.SuspendLayout();
            this._theSubSystemGroupBox.SuspendLayout();
            this.TrafodionPanel3.SuspendLayout();
            this._theSeverityGroupBox.SuspendLayout();
            this.TrafodionPanel4.SuspendLayout();
            this._theEventIdGroupBox.SuspendLayout();
            this._theProcessIdGroupBox.SuspendLayout();
            this._theMessageGroupBox.SuspendLayout();
            this.TrafodionPanel7.SuspendLayout();
            this.TrafodionPanel6.SuspendLayout();
            this._theEventTimeGroupBox.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this._theFilterButtonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theTopPanel
            // 
            this._theTopPanel.ColumnCount = 1;
            this._theTopPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this._theTopPanel.Controls.Add(this._theSubSystemGroupBox, 0, 0);
            this._theTopPanel.Controls.Add(this._theSeverityGroupBox, 0, 1);
            this._theTopPanel.Controls.Add(this._theEventIdGroupBox, 0, 2);
            this._theTopPanel.Controls.Add(this._theProcessIdGroupBox, 0, 3);
            this._theTopPanel.Controls.Add(this._theMessageGroupBox, 0, 5);
            this._theTopPanel.Controls.Add(this._theEventTimeGroupBox, 0, 4);
            this._theTopPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTopPanel.Location = new System.Drawing.Point(0, 0);
            this._theTopPanel.Name = "_theTopPanel";
            this._theTopPanel.RowCount = 6;
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 23.07692F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 23.43471F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 11.27013F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 16.10018F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 26.11807F));
            this._theTopPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 132F));
            this._theTopPanel.Size = new System.Drawing.Size(352, 720);
            this._theTopPanel.TabIndex = 0;
            // 
            // _theSubSystemGroupBox
            // 
            this._theSubSystemGroupBox.Controls.Add(this._theSubsystemsListbox);
            this._theSubSystemGroupBox.Controls.Add(this.TrafodionPanel3);
            this._theSubSystemGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSubSystemGroupBox.Location = new System.Drawing.Point(3, 3);
            this._theSubSystemGroupBox.Name = "_theSubSystemGroupBox";
            this._theSubSystemGroupBox.Size = new System.Drawing.Size(346, 129);
            this._theSubSystemGroupBox.TabIndex = 0;
            this._theSubSystemGroupBox.TabStop = false;
            this._theSubSystemGroupBox.Text = "Subsystem";
            // 
            // _theSubsystemsListbox
            // 
            this._theSubsystemsListbox.CheckOnClick = true;
            this._theSubsystemsListbox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSubsystemsListbox.FormattingEnabled = true;
            this._theSubsystemsListbox.Location = new System.Drawing.Point(3, 16);
            this._theSubsystemsListbox.Margin = new System.Windows.Forms.Padding(0, 3, 0, 3);
            this._theSubsystemsListbox.MultiColumn = true;
            this._theSubsystemsListbox.Name = "_theSubsystemsListbox";
            this._theSubsystemsListbox.Size = new System.Drawing.Size(340, 80);
            this._theSubsystemsListbox.TabIndex = 0;
            // 
            // TrafodionPanel3
            // 
            this.TrafodionPanel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel3.Controls.Add(this._theSubSystemSelectAllCheckBox);
            this.TrafodionPanel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel3.Location = new System.Drawing.Point(3, 96);
            this.TrafodionPanel3.Name = "TrafodionPanel3";
            this.TrafodionPanel3.Size = new System.Drawing.Size(340, 30);
            this.TrafodionPanel3.TabIndex = 1;
            // 
            // _theSubSystemSelectAllCheckBox
            // 
            this._theSubSystemSelectAllCheckBox.AutoSize = true;
            this._theSubSystemSelectAllCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSubSystemSelectAllCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSubSystemSelectAllCheckBox.Location = new System.Drawing.Point(10, 7);
            this._theSubSystemSelectAllCheckBox.Name = "_theSubSystemSelectAllCheckBox";
            this._theSubSystemSelectAllCheckBox.Size = new System.Drawing.Size(75, 18);
            this._theSubSystemSelectAllCheckBox.TabIndex = 0;
            this._theSubSystemSelectAllCheckBox.Text = "Select All";
            this._theSubSystemSelectAllCheckBox.UseVisualStyleBackColor = true;
            this._theSubSystemSelectAllCheckBox.CheckedChanged += new System.EventHandler(this._theSubSystemSelectAllCheckBox_CheckedChanged);
            // 
            // _theSeverityGroupBox
            // 
            this._theSeverityGroupBox.Controls.Add(this._theSeverityListBox);
            this._theSeverityGroupBox.Controls.Add(this.TrafodionPanel4);
            this._theSeverityGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSeverityGroupBox.Location = new System.Drawing.Point(3, 138);
            this._theSeverityGroupBox.Name = "_theSeverityGroupBox";
            this._theSeverityGroupBox.Size = new System.Drawing.Size(346, 131);
            this._theSeverityGroupBox.TabIndex = 1;
            this._theSeverityGroupBox.TabStop = false;
            this._theSeverityGroupBox.Text = "Severity";
            // 
            // _theSeverityListBox
            // 
            this._theSeverityListBox.CheckOnClick = true;
            this._theSeverityListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSeverityListBox.FormattingEnabled = true;
            this._theSeverityListBox.Location = new System.Drawing.Point(3, 16);
            this._theSeverityListBox.Margin = new System.Windows.Forms.Padding(6);
            this._theSeverityListBox.MultiColumn = true;
            this._theSeverityListBox.Name = "_theSeverityListBox";
            this._theSeverityListBox.Size = new System.Drawing.Size(340, 82);
            this._theSeverityListBox.TabIndex = 0;
            // 
            // TrafodionPanel4
            // 
            this.TrafodionPanel4.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel4.Controls.Add(this._theSeveritySelectAllCheckBox);
            this.TrafodionPanel4.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel4.Location = new System.Drawing.Point(3, 98);
            this.TrafodionPanel4.Name = "TrafodionPanel4";
            this.TrafodionPanel4.Size = new System.Drawing.Size(340, 30);
            this.TrafodionPanel4.TabIndex = 2;
            // 
            // _theSeveritySelectAllCheckBox
            // 
            this._theSeveritySelectAllCheckBox.AutoSize = true;
            this._theSeveritySelectAllCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSeveritySelectAllCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSeveritySelectAllCheckBox.Location = new System.Drawing.Point(10, 6);
            this._theSeveritySelectAllCheckBox.Name = "_theSeveritySelectAllCheckBox";
            this._theSeveritySelectAllCheckBox.Size = new System.Drawing.Size(75, 18);
            this._theSeveritySelectAllCheckBox.TabIndex = 1;
            this._theSeveritySelectAllCheckBox.Text = "Select All";
            this._theSeveritySelectAllCheckBox.UseVisualStyleBackColor = true;
            this._theSeveritySelectAllCheckBox.CheckedChanged += new System.EventHandler(this._theSeveritySelectAllCheckBox_CheckedChanged);
            // 
            // _theEventIdGroupBox
            // 
            this._theEventIdGroupBox.Controls.Add(this.TrafodionLabel5);
            this._theEventIdGroupBox.Controls.Add(this._theEventIdText);
            this._theEventIdGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theEventIdGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theEventIdGroupBox.Location = new System.Drawing.Point(3, 275);
            this._theEventIdGroupBox.Name = "_theEventIdGroupBox";
            this._theEventIdGroupBox.Size = new System.Drawing.Size(346, 60);
            this._theEventIdGroupBox.TabIndex = 5;
            this._theEventIdGroupBox.TabStop = false;
            this._theEventIdGroupBox.Text = "Event ID";
            // 
            // TrafodionLabel5
            // 
            this.TrafodionLabel5.AutoSize = true;
            this.TrafodionLabel5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel5.Location = new System.Drawing.Point(3, 13);
            this.TrafodionLabel5.Name = "TrafodionLabel5";
            this.TrafodionLabel5.Size = new System.Drawing.Size(285, 13);
            this.TrafodionLabel5.TabIndex = 1;
            this.TrafodionLabel5.Text = "Type an event number or numbers separated by commas:";
            // 
            // _theEventIdText
            // 
            this._theEventIdText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theEventIdText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theEventIdText.Location = new System.Drawing.Point(3, 32);
            this._theEventIdText.Name = "_theEventIdText";
            this._theEventIdText.Size = new System.Drawing.Size(337, 21);
            this._theEventIdText.TabIndex = 0;
            this._theEventIdText.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this._theEventIdText_KeyPress);
            this._theEventIdText.MouseHover += new System.EventHandler(this._theEventIdText_MouseHover);
            // 
            // _theProcessIdGroupBox
            // 
            this._theProcessIdGroupBox.Controls.Add(this.TrafodionLabel10);
            this._theProcessIdGroupBox.Controls.Add(this._theProcessNameText);
            this._theProcessIdGroupBox.Controls.Add(this.TrafodionLabel9);
            this._theProcessIdGroupBox.Controls.Add(this.TrafodionLabel8);
            this._theProcessIdGroupBox.Controls.Add(this._theProcessIdText);
            this._theProcessIdGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theProcessIdGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theProcessIdGroupBox.Location = new System.Drawing.Point(3, 341);
            this._theProcessIdGroupBox.Name = "_theProcessIdGroupBox";
            this._theProcessIdGroupBox.Size = new System.Drawing.Size(346, 88);
            this._theProcessIdGroupBox.TabIndex = 6;
            this._theProcessIdGroupBox.TabStop = false;
            this._theProcessIdGroupBox.Text = "Process";
            // 
            // TrafodionLabel10
            // 
            this.TrafodionLabel10.AutoSize = true;
            this.TrafodionLabel10.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel10.Location = new System.Drawing.Point(3, 58);
            this.TrafodionLabel10.Name = "TrafodionLabel10";
            this.TrafodionLabel10.Size = new System.Drawing.Size(87, 13);
            this.TrafodionLabel10.TabIndex = 4;
            this.TrafodionLabel10.Text = "Process Name(s)";
            // 
            // _theProcessNameText
            // 
            this._theProcessNameText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theProcessNameText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theProcessNameText.Location = new System.Drawing.Point(94, 58);
            this._theProcessNameText.Name = "_theProcessNameText";
            this._theProcessNameText.Size = new System.Drawing.Size(246, 21);
            this._theProcessNameText.TabIndex = 3;
            this._theProcessNameText.MouseHover += new System.EventHandler(this._theProcessNameText_MouseHover);
            // 
            // TrafodionLabel9
            // 
            this.TrafodionLabel9.AutoSize = true;
            this.TrafodionLabel9.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel9.Location = new System.Drawing.Point(3, 38);
            this.TrafodionLabel9.Name = "TrafodionLabel9";
            this.TrafodionLabel9.Size = new System.Drawing.Size(71, 13);
            this.TrafodionLabel9.TabIndex = 2;
            this.TrafodionLabel9.Text = "Process ID(s)";
            // 
            // TrafodionLabel8
            // 
            this.TrafodionLabel8.AutoSize = true;
            this.TrafodionLabel8.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel8.Location = new System.Drawing.Point(2, 15);
            this.TrafodionLabel8.Name = "TrafodionLabel8";
            this.TrafodionLabel8.Size = new System.Drawing.Size(282, 13);
            this.TrafodionLabel8.TabIndex = 1;
            this.TrafodionLabel8.Text = "Provide comma-separated process IDs or process names:";
            // 
            // _theProcessIdText
            // 
            this._theProcessIdText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theProcessIdText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theProcessIdText.Location = new System.Drawing.Point(94, 34);
            this._theProcessIdText.Name = "_theProcessIdText";
            this._theProcessIdText.Size = new System.Drawing.Size(246, 21);
            this._theProcessIdText.TabIndex = 0;
            this._theProcessIdText.TextChanged += new System.EventHandler(this._theProcessIdText_TextChanged);
            this._theProcessIdText.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this._theProcessIdText_KeyPress);
            this._theProcessIdText.MouseHover += new System.EventHandler(this._theProcessIdText_MouseHover);
            // 
            // _theMessageGroupBox
            // 
            this._theMessageGroupBox.Controls.Add(this.TrafodionPanel7);
            this._theMessageGroupBox.Controls.Add(this.TrafodionPanel6);
            this._theMessageGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMessageGroupBox.Location = new System.Drawing.Point(3, 588);
            this._theMessageGroupBox.Name = "_theMessageGroupBox";
            this._theMessageGroupBox.Size = new System.Drawing.Size(346, 129);
            this._theMessageGroupBox.TabIndex = 4;
            this._theMessageGroupBox.TabStop = false;
            this._theMessageGroupBox.Text = "Message";
            // 
            // TrafodionPanel7
            // 
            this.TrafodionPanel7.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel7.Controls.Add(this._theMessageFilterConditionCombo);
            this.TrafodionPanel7.Controls.Add(this.TrafodionLabel7);
            this.TrafodionPanel7.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel7.Location = new System.Drawing.Point(3, 86);
            this.TrafodionPanel7.Name = "TrafodionPanel7";
            this.TrafodionPanel7.Size = new System.Drawing.Size(340, 40);
            this.TrafodionPanel7.TabIndex = 4;
            // 
            // _theMessageFilterConditionCombo
            // 
            this._theMessageFilterConditionCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theMessageFilterConditionCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theMessageFilterConditionCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theMessageFilterConditionCombo.FormattingEnabled = true;
            this._theMessageFilterConditionCombo.Items.AddRange(new object[] {
            "Contains",
            "Does Not Contain"});
            this._theMessageFilterConditionCombo.Location = new System.Drawing.Point(91, 6);
            this._theMessageFilterConditionCombo.Name = "_theMessageFilterConditionCombo";
            this._theMessageFilterConditionCombo.Size = new System.Drawing.Size(121, 21);
            this._theMessageFilterConditionCombo.TabIndex = 1;
            // 
            // TrafodionLabel7
            // 
            this.TrafodionLabel7.AutoSize = true;
            this.TrafodionLabel7.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel7.Location = new System.Drawing.Point(7, 10);
            this.TrafodionLabel7.Name = "TrafodionLabel7";
            this.TrafodionLabel7.Size = new System.Drawing.Size(82, 13);
            this.TrafodionLabel7.TabIndex = 0;
            this.TrafodionLabel7.Text = "Filter  Condition";
            // 
            // TrafodionPanel6
            // 
            this.TrafodionPanel6.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel6.Controls.Add(this._theInCaseSensitiveCheckBox);
            this.TrafodionPanel6.Controls.Add(this._theEventText);
            this.TrafodionPanel6.Controls.Add(this.TrafodionLabel1);
            this.TrafodionPanel6.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel6.Location = new System.Drawing.Point(3, 16);
            this.TrafodionPanel6.Name = "TrafodionPanel6";
            this.TrafodionPanel6.Size = new System.Drawing.Size(340, 70);
            this.TrafodionPanel6.TabIndex = 3;
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
            // _theEventText
            // 
            this._theEventText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theEventText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theEventText.Location = new System.Drawing.Point(4, 25);
            this._theEventText.Name = "_theEventText";
            this._theEventText.Size = new System.Drawing.Size(334, 21);
            this._theEventText.TabIndex = 3;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(2, 7);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(322, 13);
            this.TrafodionLabel1.TabIndex = 4;
            this.TrafodionLabel1.Text = "Type some text that you want to find within the message column:";
            // 
            // _theEventTimeGroupBox
            // 
            this._theEventTimeGroupBox.Controls.Add(this.TrafodionPanel1);
            this._theEventTimeGroupBox.Controls.Add(this.TrafodionPanel2);
            this._theEventTimeGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theEventTimeGroupBox.Location = new System.Drawing.Point(3, 435);
            this._theEventTimeGroupBox.Name = "_theEventTimeGroupBox";
            this._theEventTimeGroupBox.Size = new System.Drawing.Size(346, 147);
            this._theEventTimeGroupBox.TabIndex = 3;
            this._theEventTimeGroupBox.TabStop = false;
            this._theEventTimeGroupBox.Text = "Event Time (Server Local Time in 24-Hour format)";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel6);
            this.TrafodionPanel1.Controls.Add(this._theEndTime);
            this.TrafodionPanel1.Controls.Add(this._theCurrentTimeCheckbox);
            this.TrafodionPanel1.Controls.Add(this._theStartTime);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel3);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel2);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(3, 67);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(340, 77);
            this.TrafodionPanel1.TabIndex = 10;
            // 
            // TrafodionLabel6
            // 
            this.TrafodionLabel6.AutoSize = true;
            this.TrafodionLabel6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel6.Location = new System.Drawing.Point(4, -1);
            this.TrafodionLabel6.Name = "TrafodionLabel6";
            this.TrafodionLabel6.Size = new System.Drawing.Size(134, 13);
            this.TrafodionLabel6.TabIndex = 10;
            this.TrafodionLabel6.Text = "OR  Select a custom range";
            // 
            // _theEndTime
            // 
            this._theEndTime.CustomFormat = "yyyy-MM-dd HH:mm:ss tt";
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
            this._theCurrentTimeCheckbox.Location = new System.Drawing.Point(250, 49);
            this._theCurrentTimeCheckbox.Name = "_theCurrentTimeCheckbox";
            this._theCurrentTimeCheckbox.Size = new System.Drawing.Size(94, 18);
            this._theCurrentTimeCheckbox.TabIndex = 9;
            this._theCurrentTimeCheckbox.Text = "Current Time";
            this._theCurrentTimeCheckbox.UseVisualStyleBackColor = true;
            this._theCurrentTimeCheckbox.CheckedChanged += new System.EventHandler(this._theCurrentTimeCheckbox_CheckedChanged);
            // 
            // _theStartTime
            // 
            this._theStartTime.CustomFormat = "yyyy-MM-dd HH:mm:ss tt";
            this._theStartTime.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this._theStartTime.Location = new System.Drawing.Point(84, 22);
            this._theStartTime.Name = "_theStartTime";
            this._theStartTime.Size = new System.Drawing.Size(154, 20);
            this._theStartTime.TabIndex = 6;
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(12, 26);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(56, 13);
            this.TrafodionLabel3.TabIndex = 8;
            this.TrafodionLabel3.Text = "Start Time";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(12, 52);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(50, 13);
            this.TrafodionLabel2.TabIndex = 7;
            this.TrafodionLabel2.Text = "End Time";
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel4);
            this.TrafodionPanel2.Controls.Add(this._theTimeRangeCombo);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel2.Location = new System.Drawing.Point(3, 16);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(340, 51);
            this.TrafodionPanel2.TabIndex = 11;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel4.Location = new System.Drawing.Point(12, 21);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(63, 13);
            this.TrafodionLabel4.TabIndex = 1;
            this.TrafodionLabel4.Text = "Time Range";
            // 
            // _theTimeRangeCombo
            // 
            this._theTimeRangeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theTimeRangeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theTimeRangeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
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
            this._theTimeRangeCombo.Size = new System.Drawing.Size(128, 21);
            this._theTimeRangeCombo.TabIndex = 0;
            this._theTimeRangeCombo.SelectedIndexChanged += new System.EventHandler(this._theTimeRangeCombo_SelectedIndexChanged);
            // 
            // _theFilterButtonPanel
            // 
            this._theFilterButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theFilterButtonPanel.Controls.Add(this._theResestButton);
            this._theFilterButtonPanel.Controls.Add(this._theApplyFilterButton);
            this._theFilterButtonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theFilterButtonPanel.Location = new System.Drawing.Point(0, 720);
            this._theFilterButtonPanel.Name = "_theFilterButtonPanel";
            this._theFilterButtonPanel.Size = new System.Drawing.Size(352, 31);
            this._theFilterButtonPanel.TabIndex = 6;
            // 
            // _theResestButton
            // 
            this._theResestButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theResestButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theResestButton.Location = new System.Drawing.Point(272, 3);
            this._theResestButton.Name = "_theResestButton";
            this._theResestButton.Size = new System.Drawing.Size(75, 23);
            this._theResestButton.TabIndex = 1;
            this._theResestButton.Text = "&Reset";
            this._theResestButton.UseVisualStyleBackColor = true;
            this._theResestButton.Click += new System.EventHandler(this._theResestButton_Click);
            // 
            // _theApplyFilterButton
            // 
            this._theApplyFilterButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theApplyFilterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theApplyFilterButton.Location = new System.Drawing.Point(191, 3);
            this._theApplyFilterButton.Name = "_theApplyFilterButton";
            this._theApplyFilterButton.Size = new System.Drawing.Size(75, 23);
            this._theApplyFilterButton.TabIndex = 0;
            this._theApplyFilterButton.Text = "&Apply Filter";
            this._theApplyFilterButton.UseVisualStyleBackColor = true;
            this._theApplyFilterButton.Click += new System.EventHandler(this._theApplyFilterButton_Click);
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // AMQPEventsFilterPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.AutoScrollMinSize = new System.Drawing.Size(340, 670);
            this.Controls.Add(this._theTopPanel);
            this.Controls.Add(this._theFilterButtonPanel);
            this.Name = "AMQPEventsFilterPanel";
            this.Size = new System.Drawing.Size(352, 751);
            this._theTopPanel.ResumeLayout(false);
            this._theSubSystemGroupBox.ResumeLayout(false);
            this.TrafodionPanel3.ResumeLayout(false);
            this.TrafodionPanel3.PerformLayout();
            this._theSeverityGroupBox.ResumeLayout(false);
            this.TrafodionPanel4.ResumeLayout(false);
            this.TrafodionPanel4.PerformLayout();
            this._theEventIdGroupBox.ResumeLayout(false);
            this._theEventIdGroupBox.PerformLayout();
            this._theProcessIdGroupBox.ResumeLayout(false);
            this._theProcessIdGroupBox.PerformLayout();
            this._theMessageGroupBox.ResumeLayout(false);
            this.TrafodionPanel7.ResumeLayout(false);
            this.TrafodionPanel7.PerformLayout();
            this.TrafodionPanel6.ResumeLayout(false);
            this.TrafodionPanel6.PerformLayout();
            this._theEventTimeGroupBox.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this._theFilterButtonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel _theTopPanel;
        private System.Windows.Forms.GroupBox _theSeverityGroupBox;
        private System.Windows.Forms.GroupBox _theEventTimeGroupBox;
        private System.Windows.Forms.GroupBox _theMessageGroupBox;
        private System.Windows.Forms.CheckedListBox _theSeverityListBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel4;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theTimeRangeCombo;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel4;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel7;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theMessageFilterConditionCombo;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel7;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel6;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _theInCaseSensitiveCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theEventText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _theSeveritySelectAllCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theEventIdGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel5;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theEventIdText;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel6;
        private System.Windows.Forms.DateTimePicker _theEndTime;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _theCurrentTimeCheckbox;
        private System.Windows.Forms.DateTimePicker _theStartTime;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private System.Windows.Forms.GroupBox _theSubSystemGroupBox;
        private System.Windows.Forms.CheckedListBox _theSubsystemsListbox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel3;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _theSubSystemSelectAllCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theFilterButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theResestButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theApplyFilterButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theProcessIdGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel8;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theProcessIdText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel10;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theProcessNameText;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel9;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _theToolTip;
    }
}
