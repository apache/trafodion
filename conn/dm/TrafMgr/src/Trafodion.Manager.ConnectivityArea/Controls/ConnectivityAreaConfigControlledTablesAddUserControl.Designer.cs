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
﻿using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaConfigControlledTablesAddUserControl
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
            Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
            this.ctIfLocked_TrafodionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.ctName_TrafodionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.browse_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.options_TrafodionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBox3 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.ctPriority_TrafodionTrackBar = new Trafodion.Manager.Framework.Controls.TrafodionTrackBar();
            this.ctPriority_TrafodionCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ctMDAM_TrafodionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBox4 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBox5 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ctTimeoutOptions_TrafodionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.ctTimeoutInterval_TrafodionNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.ctTimeoutInterval_TrafodionRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.ctTimeoutOptions_TrafodionRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.TrafodionGroupBox6 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ctTableLock_TrafodionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionGroupBox7 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ctSimilarityCheck_TrafodionComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.addCT_TrafodionToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1.SuspendLayout();
            this.options_TrafodionGroupBox.SuspendLayout();
            this.TrafodionGroupBox3.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ctPriority_TrafodionTrackBar)).BeginInit();
            this.TrafodionGroupBox2.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionGroupBox4.SuspendLayout();
            this.TrafodionGroupBox5.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ctTimeoutInterval_TrafodionNumericUpDown)).BeginInit();
            this.TrafodionGroupBox6.SuspendLayout();
            this.TrafodionGroupBox7.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionLabel1
            // 
            TrafodionLabel1.AutoSize = true;
            TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            TrafodionLabel1.Location = new System.Drawing.Point(14, 14);
            TrafodionLabel1.Name = "TrafodionLabel1";
            TrafodionLabel1.Size = new System.Drawing.Size(38, 13);
            TrafodionLabel1.TabIndex = 17;
            TrafodionLabel1.Text = "Name:";
            // 
            // ctIfLocked_TrafodionComboBox
            // 
            this.ctIfLocked_TrafodionComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.ctIfLocked_TrafodionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ctIfLocked_TrafodionComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctIfLocked_TrafodionComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctIfLocked_TrafodionComboBox.FormattingEnabled = true;
            this.ctIfLocked_TrafodionComboBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.ctIfLocked_TrafodionComboBox.Items.AddRange(new object[] {
            "<Not Set>",
            "Return",
            "Wait"});
            this.ctIfLocked_TrafodionComboBox.Location = new System.Drawing.Point(24, 21);
            this.ctIfLocked_TrafodionComboBox.Name = "ctIfLocked_TrafodionComboBox";
            this.ctIfLocked_TrafodionComboBox.Size = new System.Drawing.Size(127, 21);
            this.ctIfLocked_TrafodionComboBox.TabIndex = 7;
            this.ctIfLocked_TrafodionComboBox.SelectedIndexChanged += new System.EventHandler(this.ctIfLocked_TrafodionComboBox_SelectedIndexChanged);
            // 
            // ctName_TrafodionTextBox
            // 
            this.ctName_TrafodionTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctName_TrafodionTextBox.Location = new System.Drawing.Point(57, 11);
            this.ctName_TrafodionTextBox.Name = "ctName_TrafodionTextBox";
            this.ctName_TrafodionTextBox.Size = new System.Drawing.Size(332, 20);
            this.ctName_TrafodionTextBox.TabIndex = 0;
            this.ctName_TrafodionTextBox.TextChanged += new System.EventHandler(this.ctName_TrafodionTextBox_TextChanged);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.TrafodionPanel1.Controls.Add(TrafodionLabel1);
            this.TrafodionPanel1.Controls.Add(this.browse_TrafodionButton);
            this.TrafodionPanel1.Controls.Add(this.ctName_TrafodionTextBox);
            this.TrafodionPanel1.Location = new System.Drawing.Point(9, 28);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(497, 41);
            this.TrafodionPanel1.TabIndex = 16;
            // 
            // browse_TrafodionButton
            // 
            this.browse_TrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.browse_TrafodionButton.Location = new System.Drawing.Point(407, 9);
            this.browse_TrafodionButton.Name = "browse_TrafodionButton";
            this.browse_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.browse_TrafodionButton.TabIndex = 1;
            this.browse_TrafodionButton.Text = "Browse...";
            this.browse_TrafodionButton.UseVisualStyleBackColor = true;
            this.browse_TrafodionButton.Click += new System.EventHandler(this.browse_TrafodionButton_Click);
            // 
            // options_TrafodionGroupBox
            // 
            this.options_TrafodionGroupBox.Controls.Add(this.TrafodionGroupBox3);
            this.options_TrafodionGroupBox.Controls.Add(this.TrafodionGroupBox2);
            this.options_TrafodionGroupBox.Controls.Add(this.TrafodionGroupBox1);
            this.options_TrafodionGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.options_TrafodionGroupBox.Location = new System.Drawing.Point(9, 75);
            this.options_TrafodionGroupBox.Name = "options_TrafodionGroupBox";
            this.options_TrafodionGroupBox.Size = new System.Drawing.Size(497, 178);
            this.options_TrafodionGroupBox.TabIndex = 17;
            this.options_TrafodionGroupBox.TabStop = false;
            this.options_TrafodionGroupBox.Text = "Options:";
            // 
            // TrafodionGroupBox3
            // 
            this.TrafodionGroupBox3.Controls.Add(this.TrafodionLabel3);
            this.TrafodionGroupBox3.Controls.Add(this.TrafodionLabel2);
            this.TrafodionGroupBox3.Controls.Add(this.ctPriority_TrafodionTrackBar);
            this.TrafodionGroupBox3.Controls.Add(this.ctPriority_TrafodionCheckBox);
            this.TrafodionGroupBox3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox3.Location = new System.Drawing.Point(6, 82);
            this.TrafodionGroupBox3.Name = "TrafodionGroupBox3";
            this.TrafodionGroupBox3.Size = new System.Drawing.Size(466, 83);
            this.TrafodionGroupBox3.TabIndex = 19;
            this.TrafodionGroupBox3.TabStop = false;
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel3.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.TrafodionLabel3.Location = new System.Drawing.Point(362, 51);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(50, 13);
            this.TrafodionLabel3.TabIndex = 3;
            this.TrafodionLabel3.Text = "(highest)";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel2.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.TrafodionLabel2.Location = new System.Drawing.Point(58, 51);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(46, 13);
            this.TrafodionLabel2.TabIndex = 2;
            this.TrafodionLabel2.Text = "(lowest)";
            // 
            // ctPriority_TrafodionTrackBar
            // 
            this.ctPriority_TrafodionTrackBar.Location = new System.Drawing.Point(110, 24);
            this.ctPriority_TrafodionTrackBar.Maximum = 9;
            this.ctPriority_TrafodionTrackBar.Minimum = 1;
            this.ctPriority_TrafodionTrackBar.Name = "ctPriority_TrafodionTrackBar";
            this.ctPriority_TrafodionTrackBar.Size = new System.Drawing.Size(246, 45);
            this.ctPriority_TrafodionTrackBar.TabIndex = 1;
            this.ctPriority_TrafodionTrackBar.Value = 3;
            this.ctPriority_TrafodionTrackBar.Scroll += new System.EventHandler(this.ctPriority_TrafodionTrackBar_Scroll);
            this.ctPriority_TrafodionTrackBar.MouseUp += new System.Windows.Forms.MouseEventHandler(this.ctPriority_TrafodionTrackBar_MouseUp);
            // 
            // ctPriority_TrafodionCheckBox
            // 
            this.ctPriority_TrafodionCheckBox.AutoSize = true;
            this.ctPriority_TrafodionCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctPriority_TrafodionCheckBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctPriority_TrafodionCheckBox.Location = new System.Drawing.Point(8, 11);
            this.ctPriority_TrafodionCheckBox.Name = "ctPriority_TrafodionCheckBox";
            this.ctPriority_TrafodionCheckBox.Size = new System.Drawing.Size(66, 18);
            this.ctPriority_TrafodionCheckBox.TabIndex = 0;
            this.ctPriority_TrafodionCheckBox.Text = "Priority";
            this.ctPriority_TrafodionCheckBox.UseVisualStyleBackColor = true;
            this.ctPriority_TrafodionCheckBox.Click += new System.EventHandler(this.ctPriority_TrafodionCheckBox_Click);
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Controls.Add(this.ctMDAM_TrafodionComboBox);
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(284, 19);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(188, 61);
            this.TrafodionGroupBox2.TabIndex = 19;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "MDAM:";
            // 
            // ctMDAM_TrafodionComboBox
            // 
            this.ctMDAM_TrafodionComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.ctMDAM_TrafodionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ctMDAM_TrafodionComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctMDAM_TrafodionComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctMDAM_TrafodionComboBox.FormattingEnabled = true;
            this.ctMDAM_TrafodionComboBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.ctMDAM_TrafodionComboBox.Items.AddRange(new object[] {
            "<Not Set>",
            "Enable",
            "On",
            "Off"});
            this.ctMDAM_TrafodionComboBox.Location = new System.Drawing.Point(26, 21);
            this.ctMDAM_TrafodionComboBox.Name = "ctMDAM_TrafodionComboBox";
            this.ctMDAM_TrafodionComboBox.Size = new System.Drawing.Size(132, 21);
            this.ctMDAM_TrafodionComboBox.TabIndex = 8;
            this.ctMDAM_TrafodionComboBox.SelectedIndexChanged += new System.EventHandler(this.ctMDAM_TrafodionComboBox_SelectedIndexChanged);
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this.ctIfLocked_TrafodionComboBox);
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(6, 19);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(188, 57);
            this.TrafodionGroupBox1.TabIndex = 18;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "If-Locked:";
            // 
            // TrafodionGroupBox4
            // 
            this.TrafodionGroupBox4.Controls.Add(this.TrafodionGroupBox5);
            this.TrafodionGroupBox4.Controls.Add(this.TrafodionGroupBox6);
            this.TrafodionGroupBox4.Controls.Add(this.TrafodionGroupBox7);
            this.TrafodionGroupBox4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox4.Location = new System.Drawing.Point(9, 263);
            this.TrafodionGroupBox4.Name = "TrafodionGroupBox4";
            this.TrafodionGroupBox4.Size = new System.Drawing.Size(497, 172);
            this.TrafodionGroupBox4.TabIndex = 20;
            this.TrafodionGroupBox4.TabStop = false;
            this.TrafodionGroupBox4.Text = "System Level Options:";
            // 
            // TrafodionGroupBox5
            // 
            this.TrafodionGroupBox5.Controls.Add(this.ctTimeoutOptions_TrafodionComboBox);
            this.TrafodionGroupBox5.Controls.Add(this.TrafodionLabel4);
            this.TrafodionGroupBox5.Controls.Add(this.ctTimeoutInterval_TrafodionNumericUpDown);
            this.TrafodionGroupBox5.Controls.Add(this.ctTimeoutInterval_TrafodionRadioButton);
            this.TrafodionGroupBox5.Controls.Add(this.ctTimeoutOptions_TrafodionRadioButton);
            this.TrafodionGroupBox5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox5.Location = new System.Drawing.Point(42, 92);
            this.TrafodionGroupBox5.Name = "TrafodionGroupBox5";
            this.TrafodionGroupBox5.Size = new System.Drawing.Size(300, 71);
            this.TrafodionGroupBox5.TabIndex = 19;
            this.TrafodionGroupBox5.TabStop = false;
            this.TrafodionGroupBox5.Text = "Timeout";
            // 
            // ctTimeoutOptions_TrafodionComboBox
            // 
            this.ctTimeoutOptions_TrafodionComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.ctTimeoutOptions_TrafodionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ctTimeoutOptions_TrafodionComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctTimeoutOptions_TrafodionComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctTimeoutOptions_TrafodionComboBox.FormattingEnabled = true;
            this.ctTimeoutOptions_TrafodionComboBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.ctTimeoutOptions_TrafodionComboBox.Items.AddRange(new object[] {
            "<Not Set>",
            "No Timeout",
            "Will Not Wait"});
            this.ctTimeoutOptions_TrafodionComboBox.Location = new System.Drawing.Point(126, 15);
            this.ctTimeoutOptions_TrafodionComboBox.Name = "ctTimeoutOptions_TrafodionComboBox";
            this.ctTimeoutOptions_TrafodionComboBox.Size = new System.Drawing.Size(93, 21);
            this.ctTimeoutOptions_TrafodionComboBox.TabIndex = 9;
            this.ctTimeoutOptions_TrafodionComboBox.SelectedIndexChanged += new System.EventHandler(this.ctTimeoutOptions_TrafodionComboBox_SelectedIndexChanged);
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel4.Location = new System.Drawing.Point(214, 45);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(77, 13);
            this.TrafodionLabel4.TabIndex = 3;
            this.TrafodionLabel4.Text = "100th seconds";
            // 
            // ctTimeoutInterval_TrafodionNumericUpDown
            // 
            this.ctTimeoutInterval_TrafodionNumericUpDown.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctTimeoutInterval_TrafodionNumericUpDown.Location = new System.Drawing.Point(126, 41);
            this.ctTimeoutInterval_TrafodionNumericUpDown.Maximum = new decimal(new int[] {
            2147483647,
            0,
            0,
            0});
            this.ctTimeoutInterval_TrafodionNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.ctTimeoutInterval_TrafodionNumericUpDown.Name = "ctTimeoutInterval_TrafodionNumericUpDown";
            this.ctTimeoutInterval_TrafodionNumericUpDown.Size = new System.Drawing.Size(86, 20);
            this.ctTimeoutInterval_TrafodionNumericUpDown.TabIndex = 2;
            this.ctTimeoutInterval_TrafodionNumericUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.ctTimeoutInterval_TrafodionNumericUpDown.ValueChanged += new System.EventHandler(this.ctTimeoutInterval_TrafodionNumericUpDown_ValueChanged);
            // 
            // ctTimeoutInterval_TrafodionRadioButton
            // 
            this.ctTimeoutInterval_TrafodionRadioButton.AutoSize = true;
            this.ctTimeoutInterval_TrafodionRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctTimeoutInterval_TrafodionRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctTimeoutInterval_TrafodionRadioButton.Location = new System.Drawing.Point(64, 43);
            this.ctTimeoutInterval_TrafodionRadioButton.Name = "ctTimeoutInterval_TrafodionRadioButton";
            this.ctTimeoutInterval_TrafodionRadioButton.Size = new System.Drawing.Size(73, 18);
            this.ctTimeoutInterval_TrafodionRadioButton.TabIndex = 1;
            this.ctTimeoutInterval_TrafodionRadioButton.Text = "Interval:";
            this.ctTimeoutInterval_TrafodionRadioButton.UseVisualStyleBackColor = true;
            this.ctTimeoutInterval_TrafodionRadioButton.Click += new System.EventHandler(this.ctTimeoutInterval_TrafodionRadioButton_Click);
            // 
            // ctTimeoutOptions_TrafodionRadioButton
            // 
            this.ctTimeoutOptions_TrafodionRadioButton.AutoSize = true;
            this.ctTimeoutOptions_TrafodionRadioButton.Checked = true;
            this.ctTimeoutOptions_TrafodionRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctTimeoutOptions_TrafodionRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctTimeoutOptions_TrafodionRadioButton.Location = new System.Drawing.Point(64, 18);
            this.ctTimeoutOptions_TrafodionRadioButton.Name = "ctTimeoutOptions_TrafodionRadioButton";
            this.ctTimeoutOptions_TrafodionRadioButton.Size = new System.Drawing.Size(72, 18);
            this.ctTimeoutOptions_TrafodionRadioButton.TabIndex = 0;
            this.ctTimeoutOptions_TrafodionRadioButton.TabStop = true;
            this.ctTimeoutOptions_TrafodionRadioButton.Text = "Options:";
            this.ctTimeoutOptions_TrafodionRadioButton.UseVisualStyleBackColor = true;
            this.ctTimeoutOptions_TrafodionRadioButton.Click += new System.EventHandler(this.ctTimeoutOptions_TrafodionRadioButton_Click);
            // 
            // TrafodionGroupBox6
            // 
            this.TrafodionGroupBox6.Controls.Add(this.ctTableLock_TrafodionComboBox);
            this.TrafodionGroupBox6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox6.Location = new System.Drawing.Point(284, 22);
            this.TrafodionGroupBox6.Name = "TrafodionGroupBox6";
            this.TrafodionGroupBox6.Size = new System.Drawing.Size(188, 57);
            this.TrafodionGroupBox6.TabIndex = 19;
            this.TrafodionGroupBox6.TabStop = false;
            this.TrafodionGroupBox6.Text = "Table Lock:";
            // 
            // ctTableLock_TrafodionComboBox
            // 
            this.ctTableLock_TrafodionComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.ctTableLock_TrafodionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ctTableLock_TrafodionComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctTableLock_TrafodionComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctTableLock_TrafodionComboBox.FormattingEnabled = true;
            this.ctTableLock_TrafodionComboBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.ctTableLock_TrafodionComboBox.Items.AddRange(new object[] {
            "<Not Set>",
            "Enable",
            "On",
            "Off"});
            this.ctTableLock_TrafodionComboBox.Location = new System.Drawing.Point(11, 21);
            this.ctTableLock_TrafodionComboBox.Name = "ctTableLock_TrafodionComboBox";
            this.ctTableLock_TrafodionComboBox.Size = new System.Drawing.Size(166, 21);
            this.ctTableLock_TrafodionComboBox.TabIndex = 8;
            this.ctTableLock_TrafodionComboBox.SelectedIndexChanged += new System.EventHandler(this.ctTableLock_TrafodionComboBox_SelectedIndexChanged);
            // 
            // TrafodionGroupBox7
            // 
            this.TrafodionGroupBox7.Controls.Add(this.ctSimilarityCheck_TrafodionComboBox);
            this.TrafodionGroupBox7.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox7.Location = new System.Drawing.Point(6, 22);
            this.TrafodionGroupBox7.Name = "TrafodionGroupBox7";
            this.TrafodionGroupBox7.Size = new System.Drawing.Size(188, 57);
            this.TrafodionGroupBox7.TabIndex = 18;
            this.TrafodionGroupBox7.TabStop = false;
            this.TrafodionGroupBox7.Text = "Similarity Check:";
            // 
            // ctSimilarityCheck_TrafodionComboBox
            // 
            this.ctSimilarityCheck_TrafodionComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.ctSimilarityCheck_TrafodionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ctSimilarityCheck_TrafodionComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ctSimilarityCheck_TrafodionComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ctSimilarityCheck_TrafodionComboBox.FormattingEnabled = true;
            this.ctSimilarityCheck_TrafodionComboBox.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.ctSimilarityCheck_TrafodionComboBox.Items.AddRange(new object[] {
            "<Not Set>",
            "On",
            "Off"});
            this.ctSimilarityCheck_TrafodionComboBox.Location = new System.Drawing.Point(11, 21);
            this.ctSimilarityCheck_TrafodionComboBox.Name = "ctSimilarityCheck_TrafodionComboBox";
            this.ctSimilarityCheck_TrafodionComboBox.Size = new System.Drawing.Size(166, 21);
            this.ctSimilarityCheck_TrafodionComboBox.TabIndex = 7;
            this.ctSimilarityCheck_TrafodionComboBox.SelectedIndexChanged += new System.EventHandler(this.ctSimilarityCheck_TrafodionComboBox_SelectedIndexChanged);
            // 
            // addCT_TrafodionToolTip
            // 
            this.addCT_TrafodionToolTip.AutoPopDelay = 5000;
            this.addCT_TrafodionToolTip.InitialDelay = 200;
            this.addCT_TrafodionToolTip.IsBalloon = true;
            this.addCT_TrafodionToolTip.ReshowDelay = 100;
            // 
            // TrafodionLabel5
            // 
            this.TrafodionLabel5.AutoSize = true;
            this.TrafodionLabel5.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel5.Location = new System.Drawing.Point(61, 6);
            this.TrafodionLabel5.Name = "TrafodionLabel5";
            this.TrafodionLabel5.Size = new System.Drawing.Size(270, 13);
            this.TrafodionLabel5.TabIndex = 21;
            this.TrafodionLabel5.Text = "Please provide a name and select one or more options.";
            // 
            // ConnectivityAreaConfigControlledTablesAddUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.TrafodionLabel5);
            this.Controls.Add(this.TrafodionGroupBox4);
            this.Controls.Add(this.options_TrafodionGroupBox);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaConfigControlledTablesAddUserControl";
            this.Size = new System.Drawing.Size(518, 448);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.options_TrafodionGroupBox.ResumeLayout(false);
            this.TrafodionGroupBox3.ResumeLayout(false);
            this.TrafodionGroupBox3.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ctPriority_TrafodionTrackBar)).EndInit();
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox4.ResumeLayout(false);
            this.TrafodionGroupBox5.ResumeLayout(false);
            this.TrafodionGroupBox5.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ctTimeoutInterval_TrafodionNumericUpDown)).EndInit();
            this.TrafodionGroupBox6.ResumeLayout(false);
            this.TrafodionGroupBox7.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }




        #endregion

        private TrafodionTextBox ctName_TrafodionTextBox;
        private TrafodionComboBox ctIfLocked_TrafodionComboBox;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionButton browse_TrafodionButton;
        private TrafodionGroupBox options_TrafodionGroupBox;
        private TrafodionGroupBox TrafodionGroupBox3;
        private TrafodionGroupBox TrafodionGroupBox2;
        private TrafodionGroupBox TrafodionGroupBox1;
        private TrafodionComboBox ctMDAM_TrafodionComboBox;
        private TrafodionGroupBox TrafodionGroupBox4;
        private TrafodionGroupBox TrafodionGroupBox5;
        private TrafodionGroupBox TrafodionGroupBox6;
        private TrafodionComboBox ctTableLock_TrafodionComboBox;
        private TrafodionGroupBox TrafodionGroupBox7;
        private TrafodionComboBox ctSimilarityCheck_TrafodionComboBox;
        private TrafodionTrackBar ctPriority_TrafodionTrackBar;
        private TrafodionCheckBox ctPriority_TrafodionCheckBox;
        private TrafodionLabel TrafodionLabel2;
        private TrafodionLabel TrafodionLabel3;
        private TrafodionLabel TrafodionLabel4;
        private TrafodionNumericUpDown ctTimeoutInterval_TrafodionNumericUpDown;
        private TrafodionRadioButton ctTimeoutInterval_TrafodionRadioButton;
        private TrafodionRadioButton ctTimeoutOptions_TrafodionRadioButton;
        private TrafodionComboBox ctTimeoutOptions_TrafodionComboBox;
        public TrafodionToolTip addCT_TrafodionToolTip;
        private TrafodionLabel TrafodionLabel5;
    }
}
