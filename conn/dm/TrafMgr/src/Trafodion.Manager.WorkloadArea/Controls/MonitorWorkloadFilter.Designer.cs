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
﻿using System;
namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class MonitorWorkloadFilter
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.resetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.groupBox6 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.groupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.completedButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.rejectedButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.suspendedButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.holdingButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.waitingButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.executingButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.groupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.executingCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.waitingCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.holdingCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.suspendedCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.rejectedCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.completedCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.groupBox7 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.groupBox4 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.noWarnButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.warnLowButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.warnMediumButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.warnHighButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.groupBox3 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.warnHighCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.warnMediumCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.warnLowCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.noWarnCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.Other = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.maxWMSGraphPoints = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.maxWMSGraphPointsLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.groupBox8 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.highLightButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.sqlPreviewButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.groupBox5 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.sqlPreviewCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.highLightCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.showManageabilityCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.groupBox6.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.groupBox7.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.Other.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.maxWMSGraphPoints)).BeginInit();
            this.groupBox8.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.SuspendLayout();
            // 
            // okButton
            // 
            this.okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.okButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.okButton.Location = new System.Drawing.Point(469, 12);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(75, 23);
            this.okButton.TabIndex = 3;
            this.okButton.Text = "&OK";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // resetButton
            // 
            this.resetButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.resetButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.resetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.resetButton.Location = new System.Drawing.Point(469, 42);
            this.resetButton.Name = "resetButton";
            this.resetButton.Size = new System.Drawing.Size(75, 23);
            this.resetButton.TabIndex = 4;
            this.resetButton.Text = "&Reset";
            this.resetButton.UseVisualStyleBackColor = true;
            this.resetButton.Click += new System.EventHandler(this.resetButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(469, 72);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 5;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // groupBox6
            // 
            this.groupBox6.Controls.Add(this.groupBox2);
            this.groupBox6.Controls.Add(this.groupBox1);
            this.groupBox6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox6.Location = new System.Drawing.Point(8, 3);
            this.groupBox6.Name = "groupBox6";
            this.groupBox6.Size = new System.Drawing.Size(434, 197);
            this.groupBox6.TabIndex = 0;
            this.groupBox6.TabStop = false;
            this.groupBox6.Text = "Query States";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.completedButton);
            this.groupBox2.Controls.Add(this.rejectedButton);
            this.groupBox2.Controls.Add(this.suspendedButton);
            this.groupBox2.Controls.Add(this.holdingButton);
            this.groupBox2.Controls.Add(this.waitingButton);
            this.groupBox2.Controls.Add(this.executingButton);
            this.groupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox2.Location = new System.Drawing.Point(208, 19);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(198, 163);
            this.groupBox2.TabIndex = 1;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Foreground Color";
            // 
            // completedButton
            // 
            this.completedButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.completedButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.completedButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.completedButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.completedButton.ForeColor = System.Drawing.Color.Purple;
            this.completedButton.Location = new System.Drawing.Point(20, 131);
            this.completedButton.Name = "completedButton";
            this.completedButton.Size = new System.Drawing.Size(158, 23);
            this.completedButton.TabIndex = 5;
            this.completedButton.Text = "Completed Sample";
            this.completedButton.UseVisualStyleBackColor = false;
            this.completedButton.Click += new System.EventHandler(this.completedButton_Click);
            // 
            // rejectedButton
            // 
            this.rejectedButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.rejectedButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.rejectedButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.rejectedButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.rejectedButton.ForeColor = System.Drawing.Color.Brown;
            this.rejectedButton.Location = new System.Drawing.Point(20, 108);
            this.rejectedButton.Name = "rejectedButton";
            this.rejectedButton.Size = new System.Drawing.Size(158, 23);
            this.rejectedButton.TabIndex = 4;
            this.rejectedButton.Text = "Rejected Sample";
            this.rejectedButton.UseVisualStyleBackColor = false;
            this.rejectedButton.Click += new System.EventHandler(this.rejectedButton_Click);
            // 
            // suspendedButton
            // 
            this.suspendedButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.suspendedButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.suspendedButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.suspendedButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.suspendedButton.ForeColor = System.Drawing.Color.Gray;
            this.suspendedButton.Location = new System.Drawing.Point(20, 85);
            this.suspendedButton.Name = "suspendedButton";
            this.suspendedButton.Size = new System.Drawing.Size(158, 23);
            this.suspendedButton.TabIndex = 3;
            this.suspendedButton.Text = "Suspended Sample";
            this.suspendedButton.UseVisualStyleBackColor = false;
            this.suspendedButton.Click += new System.EventHandler(this.suspendedButton_Click);
            // 
            // holdingButton
            // 
            this.holdingButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.holdingButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.holdingButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.holdingButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.holdingButton.ForeColor = System.Drawing.Color.Gray;
            this.holdingButton.Location = new System.Drawing.Point(20, 62);
            this.holdingButton.Name = "holdingButton";
            this.holdingButton.Size = new System.Drawing.Size(158, 23);
            this.holdingButton.TabIndex = 2;
            this.holdingButton.Text = "Holding Sample";
            this.holdingButton.UseVisualStyleBackColor = false;
            this.holdingButton.Click += new System.EventHandler(this.holdingButton_Click);
            // 
            // waitingButton
            // 
            this.waitingButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.waitingButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.waitingButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.waitingButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.waitingButton.ForeColor = System.Drawing.Color.Gray;
            this.waitingButton.Location = new System.Drawing.Point(20, 39);
            this.waitingButton.Name = "waitingButton";
            this.waitingButton.Size = new System.Drawing.Size(158, 23);
            this.waitingButton.TabIndex = 1;
            this.waitingButton.Text = "Waiting Sample";
            this.waitingButton.UseVisualStyleBackColor = false;
            this.waitingButton.Click += new System.EventHandler(this.waitingButton_Click);
            // 
            // executingButton
            // 
            this.executingButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.executingButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.executingButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.executingButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.executingButton.ForeColor = System.Drawing.Color.DarkGreen;
            this.executingButton.Location = new System.Drawing.Point(20, 16);
            this.executingButton.Name = "executingButton";
            this.executingButton.Size = new System.Drawing.Size(158, 23);
            this.executingButton.TabIndex = 0;
            this.executingButton.Text = "Executing Sample";
            this.executingButton.UseVisualStyleBackColor = false;
            this.executingButton.Click += new System.EventHandler(this.executingButton_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.executingCheckBox);
            this.groupBox1.Controls.Add(this.waitingCheckBox);
            this.groupBox1.Controls.Add(this.holdingCheckBox);
            this.groupBox1.Controls.Add(this.suspendedCheckBox);
            this.groupBox1.Controls.Add(this.rejectedCheckBox);
            this.groupBox1.Controls.Add(this.completedCheckBox);
            this.groupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox1.Location = new System.Drawing.Point(20, 19);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(180, 163);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Show";
            // 
            // executingCheckBox
            // 
            this.executingCheckBox.AutoSize = true;
            this.executingCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.executingCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.executingCheckBox.Location = new System.Drawing.Point(22, 19);
            this.executingCheckBox.Name = "executingCheckBox";
            this.executingCheckBox.Size = new System.Drawing.Size(79, 18);
            this.executingCheckBox.TabIndex = 0;
            this.executingCheckBox.Text = "Executing";
            this.executingCheckBox.UseVisualStyleBackColor = true;
            this.executingCheckBox.CheckedChanged += new System.EventHandler(this.executingCheckBox_CheckedChanged);
            // 
            // waitingCheckBox
            // 
            this.waitingCheckBox.AutoSize = true;
            this.waitingCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.waitingCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.waitingCheckBox.Location = new System.Drawing.Point(22, 42);
            this.waitingCheckBox.Name = "waitingCheckBox";
            this.waitingCheckBox.Size = new System.Drawing.Size(68, 18);
            this.waitingCheckBox.TabIndex = 1;
            this.waitingCheckBox.Text = "Waiting";
            this.waitingCheckBox.UseVisualStyleBackColor = true;
            this.waitingCheckBox.CheckedChanged += new System.EventHandler(this.waitingCheckBox_CheckedChanged);
            // 
            // holdingCheckBox
            // 
            this.holdingCheckBox.AutoSize = true;
            this.holdingCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.holdingCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.holdingCheckBox.Location = new System.Drawing.Point(22, 65);
            this.holdingCheckBox.Name = "holdingCheckBox";
            this.holdingCheckBox.Size = new System.Drawing.Size(67, 18);
            this.holdingCheckBox.TabIndex = 2;
            this.holdingCheckBox.Text = "Holding";
            this.holdingCheckBox.UseVisualStyleBackColor = true;
            this.holdingCheckBox.CheckedChanged += new System.EventHandler(this.holdingCheckBox_CheckedChanged);
            // 
            // suspendedCheckBox
            // 
            this.suspendedCheckBox.AutoSize = true;
            this.suspendedCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.suspendedCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.suspendedCheckBox.Location = new System.Drawing.Point(22, 88);
            this.suspendedCheckBox.Name = "suspendedCheckBox";
            this.suspendedCheckBox.Size = new System.Drawing.Size(85, 18);
            this.suspendedCheckBox.TabIndex = 3;
            this.suspendedCheckBox.Text = "Suspended";
            this.suspendedCheckBox.UseVisualStyleBackColor = true;
            this.suspendedCheckBox.CheckedChanged += new System.EventHandler(this.suspendedCheckBox_CheckedChanged);
            // 
            // rejectedCheckBox
            // 
            this.rejectedCheckBox.AutoSize = true;
            this.rejectedCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.rejectedCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.rejectedCheckBox.Location = new System.Drawing.Point(22, 111);
            this.rejectedCheckBox.Name = "rejectedCheckBox";
            this.rejectedCheckBox.Size = new System.Drawing.Size(75, 18);
            this.rejectedCheckBox.TabIndex = 4;
            this.rejectedCheckBox.Text = "Rejected";
            this.rejectedCheckBox.UseVisualStyleBackColor = true;
            this.rejectedCheckBox.CheckedChanged += new System.EventHandler(this.rejectedCheckBox_CheckedChanged);
            // 
            // completedCheckBox
            // 
            this.completedCheckBox.AutoSize = true;
            this.completedCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.completedCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.completedCheckBox.Location = new System.Drawing.Point(22, 134);
            this.completedCheckBox.Name = "completedCheckBox";
            this.completedCheckBox.Size = new System.Drawing.Size(83, 18);
            this.completedCheckBox.TabIndex = 5;
            this.completedCheckBox.Text = "Completed";
            this.completedCheckBox.UseVisualStyleBackColor = true;
            this.completedCheckBox.CheckedChanged += new System.EventHandler(this.completedCheckBox_CheckedChanged);
            // 
            // groupBox7
            // 
            this.groupBox7.Controls.Add(this.groupBox4);
            this.groupBox7.Controls.Add(this.groupBox3);
            this.groupBox7.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox7.Location = new System.Drawing.Point(8, 205);
            this.groupBox7.Name = "groupBox7";
            this.groupBox7.Size = new System.Drawing.Size(434, 152);
            this.groupBox7.TabIndex = 1;
            this.groupBox7.TabStop = false;
            this.groupBox7.Text = "Warn Levels";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.noWarnButton);
            this.groupBox4.Controls.Add(this.warnLowButton);
            this.groupBox4.Controls.Add(this.warnMediumButton);
            this.groupBox4.Controls.Add(this.warnHighButton);
            this.groupBox4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox4.Location = new System.Drawing.Point(208, 19);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(198, 120);
            this.groupBox4.TabIndex = 1;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Background Color";
            // 
            // noWarnButton
            // 
            this.noWarnButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.noWarnButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.noWarnButton.Location = new System.Drawing.Point(20, 89);
            this.noWarnButton.Name = "noWarnButton";
            this.noWarnButton.Size = new System.Drawing.Size(158, 23);
            this.noWarnButton.TabIndex = 3;
            this.noWarnButton.Text = "NoWarn Sample";
            this.noWarnButton.UseVisualStyleBackColor = true;
            this.noWarnButton.Click += new System.EventHandler(this.noWarnButton_Click);
            // 
            // warnLowButton
            // 
            this.warnLowButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.warnLowButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.warnLowButton.Location = new System.Drawing.Point(20, 66);
            this.warnLowButton.Name = "warnLowButton";
            this.warnLowButton.Size = new System.Drawing.Size(158, 23);
            this.warnLowButton.TabIndex = 2;
            this.warnLowButton.Text = "Low Sample";
            this.warnLowButton.UseVisualStyleBackColor = true;
            this.warnLowButton.Click += new System.EventHandler(this.warnLowButton_Click);
            // 
            // warnMediumButton
            // 
            this.warnMediumButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.warnMediumButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.warnMediumButton.Location = new System.Drawing.Point(20, 43);
            this.warnMediumButton.Name = "warnMediumButton";
            this.warnMediumButton.Size = new System.Drawing.Size(158, 23);
            this.warnMediumButton.TabIndex = 1;
            this.warnMediumButton.Text = "Medium Sample";
            this.warnMediumButton.UseVisualStyleBackColor = true;
            this.warnMediumButton.Click += new System.EventHandler(this.warnMediumButton_Click);
            // 
            // warnHighButton
            // 
            this.warnHighButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.warnHighButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.warnHighButton.Location = new System.Drawing.Point(20, 20);
            this.warnHighButton.Name = "warnHighButton";
            this.warnHighButton.Size = new System.Drawing.Size(158, 23);
            this.warnHighButton.TabIndex = 0;
            this.warnHighButton.Text = "High Sample";
            this.warnHighButton.UseVisualStyleBackColor = true;
            this.warnHighButton.Click += new System.EventHandler(this.warnHighButton_Click);
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.warnHighCheckBox);
            this.groupBox3.Controls.Add(this.warnMediumCheckBox);
            this.groupBox3.Controls.Add(this.warnLowCheckBox);
            this.groupBox3.Controls.Add(this.noWarnCheckBox);
            this.groupBox3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox3.Location = new System.Drawing.Point(20, 19);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(180, 120);
            this.groupBox3.TabIndex = 0;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Show";
            // 
            // warnHighCheckBox
            // 
            this.warnHighCheckBox.AutoSize = true;
            this.warnHighCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.warnHighCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.warnHighCheckBox.Location = new System.Drawing.Point(22, 23);
            this.warnHighCheckBox.Name = "warnHighCheckBox";
            this.warnHighCheckBox.Size = new System.Drawing.Size(53, 18);
            this.warnHighCheckBox.TabIndex = 0;
            this.warnHighCheckBox.Text = "High";
            this.warnHighCheckBox.UseVisualStyleBackColor = true;
            this.warnHighCheckBox.CheckedChanged += new System.EventHandler(this.warnHighCheckBox_CheckedChanged);
            // 
            // warnMediumCheckBox
            // 
            this.warnMediumCheckBox.AutoSize = true;
            this.warnMediumCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.warnMediumCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.warnMediumCheckBox.Location = new System.Drawing.Point(22, 46);
            this.warnMediumCheckBox.Name = "warnMediumCheckBox";
            this.warnMediumCheckBox.Size = new System.Drawing.Size(68, 18);
            this.warnMediumCheckBox.TabIndex = 1;
            this.warnMediumCheckBox.Text = "Medium";
            this.warnMediumCheckBox.UseVisualStyleBackColor = true;
            this.warnMediumCheckBox.CheckedChanged += new System.EventHandler(this.warnMediumCheckBox_CheckedChanged);
            // 
            // warnLowCheckBox
            // 
            this.warnLowCheckBox.AutoSize = true;
            this.warnLowCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.warnLowCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.warnLowCheckBox.Location = new System.Drawing.Point(22, 69);
            this.warnLowCheckBox.Name = "warnLowCheckBox";
            this.warnLowCheckBox.Size = new System.Drawing.Size(51, 18);
            this.warnLowCheckBox.TabIndex = 2;
            this.warnLowCheckBox.Text = "Low";
            this.warnLowCheckBox.UseVisualStyleBackColor = true;
            this.warnLowCheckBox.CheckedChanged += new System.EventHandler(this.warnLowCheckBox_CheckedChanged);
            // 
            // noWarnCheckBox
            // 
            this.noWarnCheckBox.AutoSize = true;
            this.noWarnCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.noWarnCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.noWarnCheckBox.Location = new System.Drawing.Point(22, 92);
            this.noWarnCheckBox.Name = "noWarnCheckBox";
            this.noWarnCheckBox.Size = new System.Drawing.Size(71, 18);
            this.noWarnCheckBox.TabIndex = 3;
            this.noWarnCheckBox.Text = "NoWarn";
            this.noWarnCheckBox.UseVisualStyleBackColor = true;
            this.noWarnCheckBox.CheckedChanged += new System.EventHandler(this.noWarnCheckBox_CheckedChanged);
            // 
            // Other
            // 
            this.Other.Controls.Add(this.maxWMSGraphPoints);
            this.Other.Controls.Add(this.maxWMSGraphPointsLabel);
            this.Other.Controls.Add(this.groupBox8);
            this.Other.Controls.Add(this.groupBox5);
            this.Other.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.Other.Location = new System.Drawing.Point(8, 362);
            this.Other.Name = "Other";
            this.Other.Size = new System.Drawing.Size(434, 174);
            this.Other.TabIndex = 2;
            this.Other.TabStop = false;
            this.Other.Text = "Other Settings";
            // 
            // maxWMSGraphPoints
            // 
            this.maxWMSGraphPoints.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.maxWMSGraphPoints.Location = new System.Drawing.Point(112, 137);
            this.maxWMSGraphPoints.Maximum = new decimal(new int[] {
            2147483647,
            0,
            0,
            0});
            this.maxWMSGraphPoints.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.maxWMSGraphPoints.Name = "maxWMSGraphPoints";
            this.maxWMSGraphPoints.Size = new System.Drawing.Size(88, 21);
            this.maxWMSGraphPoints.TabIndex = 30;
            this.maxWMSGraphPoints.Value = new decimal(new int[] {
            100,
            0,
            0,
            0});
            // 
            // maxWMSGraphPointsLabel
            // 
            this.maxWMSGraphPointsLabel.AutoSize = true;
            this.maxWMSGraphPointsLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.maxWMSGraphPointsLabel.Location = new System.Drawing.Point(32, 140);
            this.maxWMSGraphPointsLabel.Name = "maxWMSGraphPointsLabel";
            this.maxWMSGraphPointsLabel.Size = new System.Drawing.Size(60, 13);
            this.maxWMSGraphPointsLabel.TabIndex = 29;
            this.maxWMSGraphPointsLabel.Text = "&Live Range";
            // 
            // groupBox8
            // 
            this.groupBox8.Controls.Add(this.highLightButton);
            this.groupBox8.Controls.Add(this.sqlPreviewButton);
            this.groupBox8.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox8.Location = new System.Drawing.Point(208, 21);
            this.groupBox8.Name = "groupBox8";
            this.groupBox8.Size = new System.Drawing.Size(198, 105);
            this.groupBox8.TabIndex = 3;
            this.groupBox8.TabStop = false;
            this.groupBox8.Text = "Foreground Color";
            // 
            // highLightButton
            // 
            this.highLightButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.highLightButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.highLightButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.highLightButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.highLightButton.ForeColor = System.Drawing.Color.Blue;
            this.highLightButton.Location = new System.Drawing.Point(20, 44);
            this.highLightButton.Name = "highLightButton";
            this.highLightButton.Size = new System.Drawing.Size(158, 23);
            this.highLightButton.TabIndex = 1;
            this.highLightButton.Text = "Highlight Changes Sample";
            this.highLightButton.UseVisualStyleBackColor = false;
            this.highLightButton.Click += new System.EventHandler(this.highLightButton_Click);
            // 
            // sqlPreviewButton
            // 
            this.sqlPreviewButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.sqlPreviewButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.sqlPreviewButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.sqlPreviewButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sqlPreviewButton.ForeColor = System.Drawing.Color.DimGray;
            this.sqlPreviewButton.Location = new System.Drawing.Point(20, 21);
            this.sqlPreviewButton.Name = "sqlPreviewButton";
            this.sqlPreviewButton.Size = new System.Drawing.Size(158, 23);
            this.sqlPreviewButton.TabIndex = 0;
            this.sqlPreviewButton.Text = "SQL Preview Sample";
            this.sqlPreviewButton.UseVisualStyleBackColor = false;
            this.sqlPreviewButton.Click += new System.EventHandler(this.sqlPreviewButton_Click);
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.sqlPreviewCheckBox);
            this.groupBox5.Controls.Add(this.highLightCheckBox);
            this.groupBox5.Controls.Add(this.showManageabilityCheckBox);
            this.groupBox5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox5.Location = new System.Drawing.Point(20, 21);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(180, 105);
            this.groupBox5.TabIndex = 2;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Show";
            // 
            // sqlPreviewCheckBox
            // 
            this.sqlPreviewCheckBox.AutoSize = true;
            this.sqlPreviewCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sqlPreviewCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sqlPreviewCheckBox.Location = new System.Drawing.Point(15, 25);
            this.sqlPreviewCheckBox.Name = "sqlPreviewCheckBox";
            this.sqlPreviewCheckBox.Size = new System.Drawing.Size(92, 18);
            this.sqlPreviewCheckBox.TabIndex = 0;
            this.sqlPreviewCheckBox.Text = "SQL Preview";
            this.sqlPreviewCheckBox.UseVisualStyleBackColor = true;
            this.sqlPreviewCheckBox.CheckedChanged += new System.EventHandler(this.sqlPreviewCheckBox_CheckedChanged);
            // 
            // highLightCheckBox
            // 
            this.highLightCheckBox.AutoSize = true;
            this.highLightCheckBox.Checked = true;
            this.highLightCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.highLightCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.highLightCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.highLightCheckBox.Location = new System.Drawing.Point(15, 48);
            this.highLightCheckBox.Name = "highLightCheckBox";
            this.highLightCheckBox.Size = new System.Drawing.Size(118, 18);
            this.highLightCheckBox.TabIndex = 1;
            this.highLightCheckBox.Text = "Highlight Changes";
            this.highLightCheckBox.UseVisualStyleBackColor = true;
            this.highLightCheckBox.CheckedChanged += new System.EventHandler(this.highLightCheckBox_CheckedChanged);
            // 
            // showManageabilityCheckBox
            // 
            this.showManageabilityCheckBox.AutoSize = true;
            this.showManageabilityCheckBox.Checked = true;
            this.showManageabilityCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.showManageabilityCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.showManageabilityCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.showManageabilityCheckBox.Location = new System.Drawing.Point(15, 72);
            this.showManageabilityCheckBox.Name = "showManageabilityCheckBox";
            this.showManageabilityCheckBox.Size = new System.Drawing.Size(138, 18);
            this.showManageabilityCheckBox.TabIndex = 4;
            this.showManageabilityCheckBox.Text = "Manageability Queries";
            this.showManageabilityCheckBox.UseVisualStyleBackColor = true;
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(469, 102);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 23);
            this.helpButton.TabIndex = 5;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // MonitorWorkloadFilter
            // 
            this.AcceptButton = this.okButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.cancelButton;
            this.ClientSize = new System.Drawing.Size(556, 558);
            this.Controls.Add(this.Other);
            this.Controls.Add(this.groupBox7);
            this.Controls.Add(this.groupBox6);
            this.Controls.Add(this.helpButton);
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.resetButton);
            this.Controls.Add(this.okButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MonitorWorkloadFilter";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Query Monitoring Options";
            this.groupBox6.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox7.ResumeLayout(false);
            this.groupBox4.ResumeLayout(false);
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.Other.ResumeLayout(false);
            this.Other.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.maxWMSGraphPoints)).EndInit();
            this.groupBox8.ResumeLayout(false);
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionButton okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton resetButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox6;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox holdingCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox waitingCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox executingCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox completedCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox rejectedCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox suspendedCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton completedButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton rejectedButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton suspendedButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton holdingButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton waitingButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton executingButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox7;
        private Trafodion.Manager.Framework.Controls.TrafodionButton noWarnButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton warnLowButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton warnMediumButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton warnHighButton;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox noWarnCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox warnLowCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox warnMediumCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox warnHighCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox4;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox3;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox Other;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox highLightCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox sqlPreviewCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox8;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox5;
        private Trafodion.Manager.Framework.Controls.TrafodionButton highLightButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton sqlPreviewButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox showManageabilityCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown maxWMSGraphPoints;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel maxWMSGraphPointsLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _theToolTip;
    }
}