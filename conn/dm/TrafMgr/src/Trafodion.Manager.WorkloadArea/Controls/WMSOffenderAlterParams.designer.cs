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
﻿namespace Trafodion.Manager.WorkloadArea.Controls
{
	partial class WMSOffenderAlterParams
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
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.alterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.sampleCacheNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.label3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.sampleCPUNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.sampleCPULabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.sampleIntervalNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.groupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.commandPreviewTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.resetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.intervalLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.cpuLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.cacheLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.serverGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.processLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.ClientGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.statusCommandGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.cpuRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.memoryRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.processTypeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.sqlRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.allRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            ((System.ComponentModel.ISupportInitialize)(this.sampleCacheNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.sampleCPUNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.sampleIntervalNumericUpDown)).BeginInit();
            this.groupBox2.SuspendLayout();
            this.serverGroupBox.SuspendLayout();
            this.ClientGroupBox.SuspendLayout();
            this.statusCommandGroupBox.SuspendLayout();
            this.processTypeGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // cancelButton
            // 
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(257, 349);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 4;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // alterButton
            // 
            this.alterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.alterButton.Location = new System.Drawing.Point(178, 349);
            this.alterButton.Name = "alterButton";
            this.alterButton.Size = new System.Drawing.Size(75, 23);
            this.alterButton.TabIndex = 2;
            this.alterButton.Text = "&OK";
            this.alterButton.UseVisualStyleBackColor = true;
            this.alterButton.Click += new System.EventHandler(this.alterButton_Click);
            // 
            // sampleCacheNumericUpDown
            // 
            this.sampleCacheNumericUpDown.Location = new System.Drawing.Point(244, 67);
            this.sampleCacheNumericUpDown.Minimum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.sampleCacheNumericUpDown.Name = "sampleCacheNumericUpDown";
            this.sampleCacheNumericUpDown.Size = new System.Drawing.Size(111, 21);
            this.sampleCacheNumericUpDown.TabIndex = 8;
            this.sampleCacheNumericUpDown.Value = new decimal(new int[] {
            10,
            0,
            0,
            0});
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label3.Location = new System.Drawing.Point(20, 71);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(144, 13);
            this.label3.TabIndex = 6;
            this.label3.Text = "Sample Cache (# processes)";
            // 
            // sampleCPUNumericUpDown
            // 
            this.sampleCPUNumericUpDown.Location = new System.Drawing.Point(244, 43);
            this.sampleCPUNumericUpDown.Maximum = new decimal(new int[] {
            16,
            0,
            0,
            0});
            this.sampleCPUNumericUpDown.Minimum = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.sampleCPUNumericUpDown.Name = "sampleCPUNumericUpDown";
            this.sampleCPUNumericUpDown.Size = new System.Drawing.Size(111, 21);
            this.sampleCPUNumericUpDown.TabIndex = 5;
            this.sampleCPUNumericUpDown.Value = new decimal(new int[] {
            2,
            0,
            0,
            0});
            // 
            // sampleCPULabel
            // 
            this.sampleCPULabel.AutoSize = true;
            this.sampleCPULabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sampleCPULabel.Location = new System.Drawing.Point(20, 47);
            this.sampleCPULabel.Name = "sampleCPULabel";
            this.sampleCPULabel.Size = new System.Drawing.Size(153, 13);
            this.sampleCPULabel.TabIndex = 3;
            this.sampleCPULabel.Text = "Sample CPUs (# CPUs/sample)";
            // 
            // sampleIntervalNumericUpDown
            // 
            this.sampleIntervalNumericUpDown.Location = new System.Drawing.Point(244, 19);
            this.sampleIntervalNumericUpDown.Maximum = new decimal(new int[] {
            60,
            0,
            0,
            0});
            this.sampleIntervalNumericUpDown.Minimum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.sampleIntervalNumericUpDown.Name = "sampleIntervalNumericUpDown";
            this.sampleIntervalNumericUpDown.Size = new System.Drawing.Size(111, 21);
            this.sampleIntervalNumericUpDown.TabIndex = 2;
            this.sampleIntervalNumericUpDown.Value = new decimal(new int[] {
            10,
            0,
            0,
            0});
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label1.Location = new System.Drawing.Point(21, 23);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(132, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Sample Interval (seconds)";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.commandPreviewTextBox);
            this.groupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox2.Location = new System.Drawing.Point(20, 100);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(335, 95);
            this.groupBox2.TabIndex = 9;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Command";
            // 
            // commandPreviewTextBox
            // 
            this.commandPreviewTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.commandPreviewTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.commandPreviewTextBox.Location = new System.Drawing.Point(3, 17);
            this.commandPreviewTextBox.Name = "commandPreviewTextBox";
            this.commandPreviewTextBox.ReadOnly = true;
            this.commandPreviewTextBox.Size = new System.Drawing.Size(329, 75);
            this.commandPreviewTextBox.TabIndex = 0;
            this.commandPreviewTextBox.Text = "";
            // 
            // resetButton
            // 
            this.resetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.resetButton.Location = new System.Drawing.Point(2, 349);
            this.resetButton.Name = "resetButton";
            this.resetButton.Size = new System.Drawing.Size(75, 23);
            this.resetButton.TabIndex = 3;
            this.resetButton.Text = "&Reset";
            this.resetButton.UseVisualStyleBackColor = true;
            this.resetButton.Click += new System.EventHandler(this.resetButton_Click);
            // 
            // intervalLinkLabel
            // 
            this.intervalLinkLabel.AutoSize = true;
            this.intervalLinkLabel.Location = new System.Drawing.Point(184, 23);
            this.intervalLinkLabel.Name = "intervalLinkLabel";
            this.intervalLinkLabel.Size = new System.Drawing.Size(12, 13);
            this.intervalLinkLabel.TabIndex = 1;
            this.intervalLinkLabel.TabStop = true;
            this.intervalLinkLabel.Text = "?";
            // 
            // cpuLinkLabel
            // 
            this.cpuLinkLabel.AutoSize = true;
            this.cpuLinkLabel.Location = new System.Drawing.Point(184, 47);
            this.cpuLinkLabel.Name = "cpuLinkLabel";
            this.cpuLinkLabel.Size = new System.Drawing.Size(12, 13);
            this.cpuLinkLabel.TabIndex = 4;
            this.cpuLinkLabel.TabStop = true;
            this.cpuLinkLabel.Text = "?";
            // 
            // cacheLinkLabel
            // 
            this.cacheLinkLabel.AutoSize = true;
            this.cacheLinkLabel.Location = new System.Drawing.Point(184, 71);
            this.cacheLinkLabel.Name = "cacheLinkLabel";
            this.cacheLinkLabel.Size = new System.Drawing.Size(12, 13);
            this.cacheLinkLabel.TabIndex = 7;
            this.cacheLinkLabel.TabStop = true;
            this.cacheLinkLabel.Text = "?";
            // 
            // toolTip1
            // 
            this.toolTip1.IsBalloon = true;
            // 
            // serverGroupBox
            // 
            this.serverGroupBox.Controls.Add(this.label1);
            this.serverGroupBox.Controls.Add(this.cacheLinkLabel);
            this.serverGroupBox.Controls.Add(this.sampleIntervalNumericUpDown);
            this.serverGroupBox.Controls.Add(this.cpuLinkLabel);
            this.serverGroupBox.Controls.Add(this.sampleCPULabel);
            this.serverGroupBox.Controls.Add(this.intervalLinkLabel);
            this.serverGroupBox.Controls.Add(this.sampleCPUNumericUpDown);
            this.serverGroupBox.Controls.Add(this.label3);
            this.serverGroupBox.Controls.Add(this.groupBox2);
            this.serverGroupBox.Controls.Add(this.sampleCacheNumericUpDown);
            this.serverGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.serverGroupBox.Location = new System.Drawing.Point(15, 12);
            this.serverGroupBox.Name = "serverGroupBox";
            this.serverGroupBox.Size = new System.Drawing.Size(380, 209);
            this.serverGroupBox.TabIndex = 0;
            this.serverGroupBox.TabStop = false;
            this.serverGroupBox.Text = "Server Parameters";
            // 
            // processLinkLabel
            // 
            this.processLinkLabel.AutoSize = true;
            this.processLinkLabel.Location = new System.Drawing.Point(130, 24);
            this.processLinkLabel.Name = "processLinkLabel";
            this.processLinkLabel.Size = new System.Drawing.Size(12, 13);
            this.processLinkLabel.TabIndex = 2;
            this.processLinkLabel.TabStop = true;
            this.processLinkLabel.Text = "?";
            // 
            // ClientGroupBox
            // 
            this.ClientGroupBox.Controls.Add(this.statusCommandGroupBox);
            this.ClientGroupBox.Controls.Add(this.processTypeGroupBox);
            this.ClientGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.ClientGroupBox.Location = new System.Drawing.Point(15, 236);
            this.ClientGroupBox.Name = "ClientGroupBox";
            this.ClientGroupBox.Size = new System.Drawing.Size(380, 103);
            this.ClientGroupBox.TabIndex = 1;
            this.ClientGroupBox.TabStop = false;
            this.ClientGroupBox.Text = "Client Parameters";
            // 
            // statusCommandGroupBox
            // 
            this.statusCommandGroupBox.Controls.Add(this.cpuRadioButton);
            this.statusCommandGroupBox.Controls.Add(this.memoryRadioButton);
            this.statusCommandGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.statusCommandGroupBox.Location = new System.Drawing.Point(194, 25);
            this.statusCommandGroupBox.Name = "statusCommandGroupBox";
            this.statusCommandGroupBox.Size = new System.Drawing.Size(161, 58);
            this.statusCommandGroupBox.TabIndex = 3;
            this.statusCommandGroupBox.TabStop = false;
            this.statusCommandGroupBox.Text = "Status Command";
            // 
            // cpuRadioButton
            // 
            this.cpuRadioButton.AutoSize = true;
            this.cpuRadioButton.Checked = true;
            this.cpuRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.cpuRadioButton.Location = new System.Drawing.Point(22, 24);
            this.cpuRadioButton.Name = "cpuRadioButton";
            this.cpuRadioButton.Size = new System.Drawing.Size(51, 18);
            this.cpuRadioButton.TabIndex = 0;
            this.cpuRadioButton.TabStop = true;
            this.cpuRadioButton.Text = "CPU";
            this.cpuRadioButton.UseVisualStyleBackColor = true;
            // 
            // memoryRadioButton
            // 
            this.memoryRadioButton.AutoSize = true;
            this.memoryRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.memoryRadioButton.Location = new System.Drawing.Point(79, 24);
            this.memoryRadioButton.Name = "memoryRadioButton";
            this.memoryRadioButton.Size = new System.Drawing.Size(69, 18);
            this.memoryRadioButton.TabIndex = 1;
            this.memoryRadioButton.Text = "Memory";
            this.memoryRadioButton.UseVisualStyleBackColor = true;
            // 
            // processTypeGroupBox
            // 
            this.processTypeGroupBox.Controls.Add(this.sqlRadioButton);
            this.processTypeGroupBox.Controls.Add(this.processLinkLabel);
            this.processTypeGroupBox.Controls.Add(this.allRadioButton);
            this.processTypeGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.processTypeGroupBox.Location = new System.Drawing.Point(20, 25);
            this.processTypeGroupBox.Name = "processTypeGroupBox";
            this.processTypeGroupBox.Size = new System.Drawing.Size(161, 58);
            this.processTypeGroupBox.TabIndex = 0;
            this.processTypeGroupBox.TabStop = false;
            this.processTypeGroupBox.Text = "Process Type";
            // 
            // sqlRadioButton
            // 
            this.sqlRadioButton.AutoSize = true;
            this.sqlRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sqlRadioButton.Location = new System.Drawing.Point(18, 24);
            this.sqlRadioButton.Name = "sqlRadioButton";
            this.sqlRadioButton.Size = new System.Drawing.Size(50, 18);
            this.sqlRadioButton.TabIndex = 0;
            this.sqlRadioButton.Text = "SQL";
            this.sqlRadioButton.UseVisualStyleBackColor = true;
            // 
            // allRadioButton
            // 
            this.allRadioButton.AutoSize = true;
            this.allRadioButton.Checked = true;
            this.allRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.allRadioButton.Location = new System.Drawing.Point(75, 24);
            this.allRadioButton.Name = "allRadioButton";
            this.allRadioButton.Size = new System.Drawing.Size(48, 18);
            this.allRadioButton.TabIndex = 1;
            this.allRadioButton.TabStop = true;
            this.allRadioButton.Text = "ALL";
            this.allRadioButton.UseVisualStyleBackColor = true;
            // 
            // helpButton
            // 
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(336, 349);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 23);
            this.helpButton.TabIndex = 4;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // WMSOffenderAlterParams
            // 
            this.AcceptButton = this.alterButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.cancelButton;
            this.ClientSize = new System.Drawing.Size(413, 375);
            this.Controls.Add(this.ClientGroupBox);
            this.Controls.Add(this.serverGroupBox);
            this.Controls.Add(this.alterButton);
            this.Controls.Add(this.helpButton);
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.resetButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "WMSOffenderAlterParams";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Alter Offender Configuration";
            ((System.ComponentModel.ISupportInitialize)(this.sampleCacheNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.sampleCPUNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.sampleIntervalNumericUpDown)).EndInit();
            this.groupBox2.ResumeLayout(false);
            this.serverGroupBox.ResumeLayout(false);
            this.serverGroupBox.PerformLayout();
            this.ClientGroupBox.ResumeLayout(false);
            this.statusCommandGroupBox.ResumeLayout(false);
            this.statusCommandGroupBox.PerformLayout();
            this.processTypeGroupBox.ResumeLayout(false);
            this.processTypeGroupBox.PerformLayout();
            this.ResumeLayout(false);

		}

		#endregion

		private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
		private Trafodion.Manager.Framework.Controls.TrafodionButton alterButton;
		private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown sampleCacheNumericUpDown;
		private Trafodion.Manager.Framework.Controls.TrafodionLabel label3;
		private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown sampleCPUNumericUpDown;
		private Trafodion.Manager.Framework.Controls.TrafodionLabel sampleCPULabel;
		private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown sampleIntervalNumericUpDown;
		private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
		private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox2;
		private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox commandPreviewTextBox;
		private Trafodion.Manager.Framework.Controls.TrafodionButton resetButton;
		private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel intervalLinkLabel;
		private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel cpuLinkLabel;
		private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel cacheLinkLabel;
		private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
		private Trafodion.Manager.Framework.Controls.TrafodionGroupBox serverGroupBox;
		private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel processLinkLabel;
		private Trafodion.Manager.Framework.Controls.TrafodionGroupBox ClientGroupBox;
		private Trafodion.Manager.Framework.Controls.TrafodionRadioButton allRadioButton;
		private Trafodion.Manager.Framework.Controls.TrafodionRadioButton sqlRadioButton;
		private Trafodion.Manager.Framework.Controls.TrafodionGroupBox processTypeGroupBox;
		private Trafodion.Manager.Framework.Controls.TrafodionGroupBox statusCommandGroupBox;
		private Trafodion.Manager.Framework.Controls.TrafodionRadioButton cpuRadioButton;
		private Trafodion.Manager.Framework.Controls.TrafodionRadioButton memoryRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
	}
}