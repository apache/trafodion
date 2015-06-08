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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class RunScriptDialog
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
            this._thePanelScript = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._lblErrorConnection = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblRequiredMark = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theRunOnNodeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theNodeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theStandardOutErrorCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._executeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._tableNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._parametersTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._scriptNameComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this.TrafodionBannerControl1 = new Trafodion.Manager.Framework.Controls.TrafodionBannerControl();
            this._thePanelButtons = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theCloseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theHelpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._thePanelOutput = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theGrpOutput = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._outputTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.runscriptTooltips = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._thePanelScript.SuspendLayout();
            this.statusStrip.SuspendLayout();
            this._thePanelButtons.SuspendLayout();
            this._thePanelOutput.SuspendLayout();
            this._theGrpOutput.SuspendLayout();
            this.SuspendLayout();
            // 
            // _thePanelScript
            // 
            this._thePanelScript.BackColor = System.Drawing.Color.WhiteSmoke;
            this._thePanelScript.Controls.Add(this._lblErrorConnection);
            this._thePanelScript.Controls.Add(this.lblRequiredMark);
            this._thePanelScript.Controls.Add(this._theRunOnNodeLabel);
            this._thePanelScript.Controls.Add(this._theNodeTextBox);
            this._thePanelScript.Controls.Add(this._theStandardOutErrorCheckBox);
            this._thePanelScript.Controls.Add(this._executeButton);
            this._thePanelScript.Controls.Add(this._tableNameTextBox);
            this._thePanelScript.Controls.Add(this._parametersTextBox);
            this._thePanelScript.Controls.Add(this._scriptNameComboBox);
            this._thePanelScript.Controls.Add(this.TrafodionLabel3);
            this._thePanelScript.Controls.Add(this.TrafodionLabel2);
            this._thePanelScript.Controls.Add(this.TrafodionLabel1);
            this._thePanelScript.Dock = System.Windows.Forms.DockStyle.Top;
            this._thePanelScript.Location = new System.Drawing.Point(0, 51);
            this._thePanelScript.Name = "_thePanelScript";
            this._thePanelScript.Size = new System.Drawing.Size(968, 118);
            this._thePanelScript.TabIndex = 0;
            // 
            // _lblErrorConnection
            // 
            this._lblErrorConnection.AutoSize = true;
            this._lblErrorConnection.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._lblErrorConnection.ForeColor = System.Drawing.Color.Red;
            this._lblErrorConnection.Location = new System.Drawing.Point(474, 94);
            this._lblErrorConnection.Name = "_lblErrorConnection";
            this._lblErrorConnection.Size = new System.Drawing.Size(0, 13);
            this._lblErrorConnection.TabIndex = 9;
            // 
            // lblRequiredMark
            // 
            this.lblRequiredMark.AutoSize = true;
            this.lblRequiredMark.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblRequiredMark.ForeColor = System.Drawing.Color.Red;
            this.lblRequiredMark.Location = new System.Drawing.Point(110, 16);
            this.lblRequiredMark.Name = "lblRequiredMark";
            this.lblRequiredMark.Size = new System.Drawing.Size(13, 13);
            this.lblRequiredMark.TabIndex = 8;
            this.lblRequiredMark.Text = "*";
            // 
            // _theRunOnNodeLabel
            // 
            this._theRunOnNodeLabel.AutoSize = true;
            this._theRunOnNodeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRunOnNodeLabel.Location = new System.Drawing.Point(22, 41);
            this._theRunOnNodeLabel.Name = "_theRunOnNodeLabel";
            this._theRunOnNodeLabel.Size = new System.Drawing.Size(88, 13);
            this._theRunOnNodeLabel.TabIndex = 7;
            this._theRunOnNodeLabel.Text = "Run on the Node";
            // 
            // _theNodeTextBox
            // 
            this._theNodeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theNodeTextBox.Location = new System.Drawing.Point(124, 38);
            this._theNodeTextBox.Name = "_theNodeTextBox";
            this._theNodeTextBox.Size = new System.Drawing.Size(257, 21);
            this._theNodeTextBox.TabIndex = 6;
            // 
            // _theStandardOutErrorCheckBox
            // 
            this._theStandardOutErrorCheckBox.AutoSize = true;
            this._theStandardOutErrorCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theStandardOutErrorCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theStandardOutErrorCheckBox.Location = new System.Drawing.Point(392, 15);
            this._theStandardOutErrorCheckBox.Name = "_theStandardOutErrorCheckBox";
            this._theStandardOutErrorCheckBox.Size = new System.Drawing.Size(249, 18);
            this._theStandardOutErrorCheckBox.TabIndex = 4;
            this._theStandardOutErrorCheckBox.Text = "Return both standard out and standard error";
            this._theStandardOutErrorCheckBox.UseVisualStyleBackColor = true;
            // 
            // _executeButton
            // 
            this._executeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._executeButton.Location = new System.Drawing.Point(392, 89);
            this._executeButton.Name = "_executeButton";
            this._executeButton.Size = new System.Drawing.Size(75, 23);
            this._executeButton.TabIndex = 3;
            this._executeButton.Text = "&Execute";
            this._executeButton.UseVisualStyleBackColor = true;
            this._executeButton.Click += new System.EventHandler(this._executeButton_Click);
            // 
            // _tableNameTextBox
            // 
            this._tableNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._tableNameTextBox.Location = new System.Drawing.Point(124, 91);
            this._tableNameTextBox.Name = "_tableNameTextBox";
            this._tableNameTextBox.Size = new System.Drawing.Size(257, 21);
            this._tableNameTextBox.TabIndex = 2;
            // 
            // _parametersTextBox
            // 
            this._parametersTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._parametersTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._parametersTextBox.Location = new System.Drawing.Point(124, 62);
            this._parametersTextBox.Name = "_parametersTextBox";
            this._parametersTextBox.Size = new System.Drawing.Size(817, 21);
            this._parametersTextBox.TabIndex = 1;
            // 
            // _scriptNameComboBox
            // 
            this._scriptNameComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._scriptNameComboBox.FormattingEnabled = true;
            this._scriptNameComboBox.Location = new System.Drawing.Point(124, 11);
            this._scriptNameComboBox.Name = "_scriptNameComboBox";
            this._scriptNameComboBox.Size = new System.Drawing.Size(257, 21);
            this._scriptNameComboBox.TabIndex = 0;
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(9, 93);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(101, 13);
            this.TrafodionLabel3.TabIndex = 0;
            this.TrafodionLabel3.Text = "Results Table Name";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(48, 66);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(62, 13);
            this.TrafodionLabel2.TabIndex = 0;
            this.TrafodionLabel2.Text = "Parameters";
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(46, 14);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(64, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Script Name";
            // 
            // statusStrip
            // 
            this.statusStrip.BackColor = System.Drawing.Color.WhiteSmoke;
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel,
            this.toolStripProgressBar});
            this.statusStrip.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.HorizontalStackWithOverflow;
            this.statusStrip.Location = new System.Drawing.Point(0, 642);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(968, 22);
            this.statusStrip.TabIndex = 2;
            this.statusStrip.Text = "statusStrip1";
            // 
            // toolStripStatusLabel
            // 
            this.toolStripStatusLabel.AutoSize = false;
            this.toolStripStatusLabel.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
            this.toolStripStatusLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this.toolStripStatusLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.toolStripStatusLabel.Name = "toolStripStatusLabel";
            this.toolStripStatusLabel.Padding = new System.Windows.Forms.Padding(0, 0, 20, 0);
            this.toolStripStatusLabel.Size = new System.Drawing.Size(400, 17);
            this.toolStripStatusLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // toolStripProgressBar
            // 
            this.toolStripProgressBar.AutoSize = false;
            this.toolStripProgressBar.Name = "toolStripProgressBar";
            this.toolStripProgressBar.Padding = new System.Windows.Forms.Padding(10, 0, 0, 0);
            this.toolStripProgressBar.Size = new System.Drawing.Size(200, 16);
            this.toolStripProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // TrafodionBannerControl1
            // 
            this.TrafodionBannerControl1.ConnectionDefinition = null;
            this.TrafodionBannerControl1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionBannerControl1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionBannerControl1.Name = "TrafodionBannerControl1";
            this.TrafodionBannerControl1.ShowDescription = true;
            this.TrafodionBannerControl1.Size = new System.Drawing.Size(968, 51);
            this.TrafodionBannerControl1.TabIndex = 4;
            // 
            // _thePanelButtons
            // 
            this._thePanelButtons.BackColor = System.Drawing.Color.WhiteSmoke;
            this._thePanelButtons.Controls.Add(this._theCloseButton);
            this._thePanelButtons.Controls.Add(this._theHelpButton);
            this._thePanelButtons.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._thePanelButtons.Location = new System.Drawing.Point(0, 606);
            this._thePanelButtons.Name = "_thePanelButtons";
            this._thePanelButtons.Size = new System.Drawing.Size(968, 36);
            this._thePanelButtons.TabIndex = 5;
            // 
            // _theCloseButton
            // 
            this._theCloseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCloseButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._theCloseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCloseButton.Location = new System.Drawing.Point(791, 7);
            this._theCloseButton.Name = "_theCloseButton";
            this._theCloseButton.Size = new System.Drawing.Size(75, 23);
            this._theCloseButton.TabIndex = 0;
            this._theCloseButton.Text = "&Close";
            this._theCloseButton.UseVisualStyleBackColor = true;
            this._theCloseButton.Click += new System.EventHandler(this._theCloseButton_Click);
            // 
            // _theHelpButton
            // 
            this._theHelpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theHelpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theHelpButton.Location = new System.Drawing.Point(881, 7);
            this._theHelpButton.Name = "_theHelpButton";
            this._theHelpButton.Size = new System.Drawing.Size(75, 23);
            this._theHelpButton.TabIndex = 0;
            this._theHelpButton.Text = "He&lp";
            this._theHelpButton.UseVisualStyleBackColor = true;
            this._theHelpButton.Click += new System.EventHandler(this._theHelpButton_Click);
            // 
            // _thePanelOutput
            // 
            this._thePanelOutput.BackColor = System.Drawing.Color.WhiteSmoke;
            this._thePanelOutput.Controls.Add(this._theGrpOutput);
            this._thePanelOutput.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePanelOutput.Location = new System.Drawing.Point(0, 169);
            this._thePanelOutput.Name = "_thePanelOutput";
            this._thePanelOutput.Size = new System.Drawing.Size(968, 437);
            this._thePanelOutput.TabIndex = 6;
            // 
            // _theGrpOutput
            // 
            this._theGrpOutput.Controls.Add(this._outputTextBox);
            this._theGrpOutput.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGrpOutput.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theGrpOutput.Location = new System.Drawing.Point(0, 0);
            this._theGrpOutput.Name = "_theGrpOutput";
            this._theGrpOutput.Size = new System.Drawing.Size(968, 437);
            this._theGrpOutput.TabIndex = 0;
            this._theGrpOutput.TabStop = false;
            this._theGrpOutput.Text = "Output";
            // 
            // _outputTextBox
            // 
            this._outputTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._outputTextBox.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._outputTextBox.Location = new System.Drawing.Point(3, 17);
            this._outputTextBox.Margin = new System.Windows.Forms.Padding(3, 3, 6, 3);
            this._outputTextBox.Name = "_outputTextBox";
            this._outputTextBox.ReadOnly = true;
            this._outputTextBox.Size = new System.Drawing.Size(962, 417);
            this._outputTextBox.TabIndex = 0;
            this._outputTextBox.Text = "";
            // 
            // runscriptTooltips
            // 
            this.runscriptTooltips.IsBalloon = true;
            // 
            // RunScriptDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(968, 664);
            this.Controls.Add(this._thePanelOutput);
            this.Controls.Add(this._thePanelButtons);
            this.Controls.Add(this._thePanelScript);
            this.Controls.Add(this.TrafodionBannerControl1);
            this.Controls.Add(this.statusStrip);
            this.Name = "RunScriptDialog";
            this.Text = "Trafodion Database Manager - Run Script";
            this._thePanelScript.ResumeLayout(false);
            this._thePanelScript.PerformLayout();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this._thePanelButtons.ResumeLayout(false);
            this._thePanelOutput.ResumeLayout(false);
            this._theGrpOutput.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionPanel _thePanelScript;
        private Framework.Controls.TrafodionTextBox _tableNameTextBox;
        private Framework.Controls.TrafodionTextBox _parametersTextBox;
        private Framework.Controls.TrafodionComboBox _scriptNameComboBox;
        private Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Framework.Controls.TrafodionButton _executeButton;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel;
        private System.Windows.Forms.ToolStripProgressBar toolStripProgressBar;
        private Framework.Controls.TrafodionBannerControl TrafodionBannerControl1;
        private Framework.Controls.TrafodionCheckBox _theStandardOutErrorCheckBox;
        private Framework.Controls.TrafodionPanel _thePanelButtons;
        private Framework.Controls.TrafodionButton _theCloseButton;
        private Framework.Controls.TrafodionButton _theHelpButton;
        private Framework.Controls.TrafodionPanel _thePanelOutput;
        private Framework.Controls.TrafodionGroupBox _theGrpOutput;
        private Framework.Controls.TrafodionRichTextBox _outputTextBox;
        private Framework.Controls.TrafodionTextBox _theNodeTextBox;
        private Framework.Controls.TrafodionLabel _theRunOnNodeLabel;
        private Framework.Controls.TrafodionLabel lblRequiredMark;
        private Framework.Controls.TrafodionToolTip runscriptTooltips;
        private Framework.Controls.TrafodionLabel _lblErrorConnection;
    }
}