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
    partial class WMSOffenderStatusCommand
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
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.statusGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.statusLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.memRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.cpuRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.cpuNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.label2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.segmentNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.commandGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.syntaxLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.commandPreviewTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.useCPUCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.useSegmentCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.processGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.processLinkLabel = new Trafodion.Manager.Framework.Controls.TrafodionLinkLabel();
            this.allRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.sqlRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.statusGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cpuNumericUpDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.segmentNumericUpDown)).BeginInit();
            this.commandGroupBox.SuspendLayout();
            this.processGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // okButton
            // 
            this.okButton.Location = new System.Drawing.Point(86, 282);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(75, 23);
            this.okButton.TabIndex = 9;
            this.okButton.Text = "&OK";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Location = new System.Drawing.Point(183, 282);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 10;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // statusGroupBox
            // 
            this.statusGroupBox.Controls.Add(this.statusLinkLabel);
            this.statusGroupBox.Controls.Add(this.memRadioButton);
            this.statusGroupBox.Controls.Add(this.cpuRadioButton);
            this.statusGroupBox.Location = new System.Drawing.Point(17, 12);
            this.statusGroupBox.Name = "statusGroupBox";
            this.statusGroupBox.Size = new System.Drawing.Size(290, 51);
            this.statusGroupBox.TabIndex = 0;
            this.statusGroupBox.TabStop = false;
            this.statusGroupBox.Text = "Status";
            // 
            // statusLinkLabel
            // 
            this.statusLinkLabel.AutoSize = true;
            this.statusLinkLabel.Location = new System.Drawing.Point(202, 21);
            this.statusLinkLabel.Name = "statusLinkLabel";
            this.statusLinkLabel.Size = new System.Drawing.Size(29, 13);
            this.statusLinkLabel.TabIndex = 2;
            this.statusLinkLabel.TabStop = true;
            this.statusLinkLabel.Text = "Help";
            // 
            // memRadioButton
            // 
            this.memRadioButton.AutoSize = true;
            this.memRadioButton.Location = new System.Drawing.Point(124, 21);
            this.memRadioButton.Name = "memRadioButton";
            this.memRadioButton.Size = new System.Drawing.Size(48, 17);
            this.memRadioButton.TabIndex = 1;
            this.memRadioButton.Text = "Mem";
            this.memRadioButton.UseVisualStyleBackColor = true;
            // 
            // cpuRadioButton
            // 
            this.cpuRadioButton.AutoSize = true;
            this.cpuRadioButton.Checked = true;
            this.cpuRadioButton.Location = new System.Drawing.Point(33, 21);
            this.cpuRadioButton.Name = "cpuRadioButton";
            this.cpuRadioButton.Size = new System.Drawing.Size(47, 17);
            this.cpuRadioButton.TabIndex = 0;
            this.cpuRadioButton.TabStop = true;
            this.cpuRadioButton.Text = "CPU";
            this.cpuRadioButton.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(196, 85);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(39, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "CPU #";
            // 
            // cpuNumericUpDown
            // 
            this.cpuNumericUpDown.Location = new System.Drawing.Point(244, 81);
            this.cpuNumericUpDown.Maximum = new decimal(new int[] {
            15,
            0,
            0,
            0});
            this.cpuNumericUpDown.Name = "cpuNumericUpDown";
            this.cpuNumericUpDown.Size = new System.Drawing.Size(60, 20);
            this.cpuNumericUpDown.TabIndex = 3;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(176, 118);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(59, 13);
            this.label2.TabIndex = 5;
            this.label2.Text = "Segment #";
            // 
            // segmentNumericUpDown
            // 
            this.segmentNumericUpDown.Location = new System.Drawing.Point(244, 114);
            this.segmentNumericUpDown.Maximum = new decimal(new int[] {
            256,
            0,
            0,
            0});
            this.segmentNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.segmentNumericUpDown.Name = "segmentNumericUpDown";
            this.segmentNumericUpDown.Size = new System.Drawing.Size(60, 20);
            this.segmentNumericUpDown.TabIndex = 6;
            this.segmentNumericUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // commandGroupBox
            // 
            this.commandGroupBox.Controls.Add(this.syntaxLinkLabel);
            this.commandGroupBox.Controls.Add(this.commandPreviewTextBox);
            this.commandGroupBox.Location = new System.Drawing.Point(17, 203);
            this.commandGroupBox.Name = "commandGroupBox";
            this.commandGroupBox.Size = new System.Drawing.Size(290, 62);
            this.commandGroupBox.TabIndex = 8;
            this.commandGroupBox.TabStop = false;
            this.commandGroupBox.Text = "Command";
            // 
            // syntaxLinkLabel
            // 
            this.syntaxLinkLabel.AutoSize = true;
            this.syntaxLinkLabel.Location = new System.Drawing.Point(232, 40);
            this.syntaxLinkLabel.Name = "syntaxLinkLabel";
            this.syntaxLinkLabel.Size = new System.Drawing.Size(39, 13);
            this.syntaxLinkLabel.TabIndex = 1;
            this.syntaxLinkLabel.TabStop = true;
            this.syntaxLinkLabel.Text = "Syntax";
            // 
            // commandPreviewTextBox
            // 
            this.commandPreviewTextBox.Location = new System.Drawing.Point(3, 16);
            this.commandPreviewTextBox.Multiline = true;
            this.commandPreviewTextBox.Name = "commandPreviewTextBox";
            this.commandPreviewTextBox.Size = new System.Drawing.Size(284, 21);
            this.commandPreviewTextBox.TabIndex = 0;
            // 
            // useCPUCheckBox
            // 
            this.useCPUCheckBox.AutoSize = true;
            this.useCPUCheckBox.Checked = true;
            this.useCPUCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.useCPUCheckBox.Location = new System.Drawing.Point(17, 82);
            this.useCPUCheckBox.Name = "useCPUCheckBox";
            this.useCPUCheckBox.Size = new System.Drawing.Size(80, 17);
            this.useCPUCheckBox.TabIndex = 1;
            this.useCPUCheckBox.Text = "Use CPU #";
            this.useCPUCheckBox.UseVisualStyleBackColor = true;
            this.useCPUCheckBox.CheckedChanged += new System.EventHandler(this.useCPUCheckBox_CheckedChanged);
            // 
            // useSegmentCheckBox
            // 
            this.useSegmentCheckBox.AutoSize = true;
            this.useSegmentCheckBox.Checked = true;
            this.useSegmentCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.useSegmentCheckBox.Location = new System.Drawing.Point(17, 115);
            this.useSegmentCheckBox.Name = "useSegmentCheckBox";
            this.useSegmentCheckBox.Size = new System.Drawing.Size(100, 17);
            this.useSegmentCheckBox.TabIndex = 4;
            this.useSegmentCheckBox.Text = "Use Segment #";
            this.useSegmentCheckBox.UseVisualStyleBackColor = true;
            this.useSegmentCheckBox.CheckedChanged += new System.EventHandler(this.useSegmentCheckBox_CheckedChanged);
            // 
            // processGroupBox
            // 
            this.processGroupBox.Controls.Add(this.processLinkLabel);
            this.processGroupBox.Controls.Add(this.allRadioButton);
            this.processGroupBox.Controls.Add(this.sqlRadioButton);
            this.processGroupBox.Location = new System.Drawing.Point(17, 141);
            this.processGroupBox.Name = "processGroupBox";
            this.processGroupBox.Size = new System.Drawing.Size(290, 51);
            this.processGroupBox.TabIndex = 7;
            this.processGroupBox.TabStop = false;
            this.processGroupBox.Text = "Process";
            // 
            // processLinkLabel
            // 
            this.processLinkLabel.AutoSize = true;
            this.processLinkLabel.Location = new System.Drawing.Point(202, 21);
            this.processLinkLabel.Name = "processLinkLabel";
            this.processLinkLabel.Size = new System.Drawing.Size(29, 13);
            this.processLinkLabel.TabIndex = 2;
            this.processLinkLabel.TabStop = true;
            this.processLinkLabel.Text = "Help";
            // 
            // allRadioButton
            // 
            this.allRadioButton.AutoSize = true;
            this.allRadioButton.Checked = true;
            this.allRadioButton.Location = new System.Drawing.Point(124, 21);
            this.allRadioButton.Name = "allRadioButton";
            this.allRadioButton.Size = new System.Drawing.Size(44, 17);
            this.allRadioButton.TabIndex = 1;
            this.allRadioButton.TabStop = true;
            this.allRadioButton.Text = "ALL";
            this.allRadioButton.UseVisualStyleBackColor = true;
            // 
            // sqlRadioButton
            // 
            this.sqlRadioButton.AutoSize = true;
            this.sqlRadioButton.Location = new System.Drawing.Point(33, 21);
            this.sqlRadioButton.Name = "sqlRadioButton";
            this.sqlRadioButton.Size = new System.Drawing.Size(46, 17);
            this.sqlRadioButton.TabIndex = 0;
            this.sqlRadioButton.Text = "SQL";
            this.sqlRadioButton.UseVisualStyleBackColor = true;
            // 
            // WMSOffenderStatusCommand
            // 
            this.AcceptButton = this.okButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.cancelButton;
            this.ClientSize = new System.Drawing.Size(325, 315);
            this.Controls.Add(this.processGroupBox);
            this.Controls.Add(this.useSegmentCheckBox);
            this.Controls.Add(this.useCPUCheckBox);
            this.Controls.Add(this.commandGroupBox);
            this.Controls.Add(this.segmentNumericUpDown);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.cpuNumericUpDown);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.statusGroupBox);
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.okButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "WMSOffenderStatusCommand";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - NVOffenderCustomizeStatusForm";
            this.statusGroupBox.ResumeLayout(false);
            this.statusGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cpuNumericUpDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.segmentNumericUpDown)).EndInit();
            this.commandGroupBox.ResumeLayout(false);
            this.commandGroupBox.PerformLayout();
            this.processGroupBox.ResumeLayout(false);
            this.processGroupBox.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionButton okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox statusGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton memRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton cpuRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown cpuNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label2;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown segmentNumericUpDown;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox commandGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox commandPreviewTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox useCPUCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox useSegmentCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox processGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton allRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton sqlRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel statusLinkLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel processLinkLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
        private Trafodion.Manager.Framework.Controls.TrafodionLinkLabel syntaxLinkLabel;
    }
}