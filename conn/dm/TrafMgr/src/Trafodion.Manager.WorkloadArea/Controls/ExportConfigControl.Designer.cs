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
﻿using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class ExportConfigControl : UserControl
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
            this.ddlGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ddlTextBoxPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.ddlOutputTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.ouptutButtonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.saveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.doneButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.ddlGroupBox.SuspendLayout();
            this.ddlTextBoxPanel.SuspendLayout();
            this.ouptutButtonsPanel.SuspendLayout();
            this.statusStrip.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // ddlGroupBox
            // 
            this.ddlGroupBox.AutoSize = true;
            this.ddlGroupBox.Controls.Add(this.ddlTextBoxPanel);
            this.ddlGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ddlGroupBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.ddlGroupBox.Location = new System.Drawing.Point(0, 0);
            this.ddlGroupBox.Name = "ddlGroupBox";
            this.ddlGroupBox.Size = new System.Drawing.Size(831, 453);
            this.ddlGroupBox.TabIndex = 2;
            this.ddlGroupBox.TabStop = false;
            this.ddlGroupBox.Text = "DDL Output";
            // 
            // ddlTextBoxPanel
            // 
            this.ddlTextBoxPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.ddlTextBoxPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(255)))), ((int)(((byte)(255)))));
            this.ddlTextBoxPanel.Controls.Add(this.ddlOutputTextBox);
            this.ddlTextBoxPanel.Location = new System.Drawing.Point(10, 26);
            this.ddlTextBoxPanel.Name = "ddlTextBoxPanel";
            this.ddlTextBoxPanel.Size = new System.Drawing.Size(815, 424);
            this.ddlTextBoxPanel.TabIndex = 0;
            // 
            // ddlOutputTextBox
            // 
            this.ddlOutputTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ddlOutputTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ddlOutputTextBox.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ddlOutputTextBox.Location = new System.Drawing.Point(0, 0);
            this.ddlOutputTextBox.Name = "ddlOutputTextBox";
            this.ddlOutputTextBox.ReadOnly = true;
            this.ddlOutputTextBox.Size = new System.Drawing.Size(815, 424);
            this.ddlOutputTextBox.TabIndex = 0;
            this.ddlOutputTextBox.Text = "";
            this.ddlOutputTextBox.WordWrap = false;
            // 
            // ouptutButtonsPanel
            // 
            this.ouptutButtonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ouptutButtonsPanel.Controls.Add(this.helpButton);
            this.ouptutButtonsPanel.Controls.Add(this.saveButton);
            this.ouptutButtonsPanel.Controls.Add(this.doneButton);
            this.ouptutButtonsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ouptutButtonsPanel.Location = new System.Drawing.Point(0, 0);
            this.ouptutButtonsPanel.Name = "ouptutButtonsPanel";
            this.ouptutButtonsPanel.Size = new System.Drawing.Size(831, 41);
            this.ouptutButtonsPanel.TabIndex = 2;
            // 
            // saveButton
            // 
            this.saveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.saveButton.Enabled = false;
            this.saveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.saveButton.Location = new System.Drawing.Point(558, 7);
            this.saveButton.Name = "saveButton";
            this.saveButton.Size = new System.Drawing.Size(85, 25);
            this.saveButton.TabIndex = 2;
            this.saveButton.Text = "&Save";
            this.saveButton.UseVisualStyleBackColor = true;
            this.saveButton.Click += new System.EventHandler(this.saveButton_Click);
            // 
            // doneButton
            // 
            this.doneButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.doneButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.doneButton.Location = new System.Drawing.Point(649, 7);
            this.doneButton.Name = "doneButton";
            this.doneButton.Size = new System.Drawing.Size(85, 25);
            this.doneButton.TabIndex = 1;
            this.doneButton.Text = "&Done";
            this.doneButton.UseVisualStyleBackColor = true;
            this.doneButton.Click += new System.EventHandler(this.doneButton_Click);
            // 
            // statusStrip
            // 
            this.statusStrip.BackColor = System.Drawing.Color.WhiteSmoke;
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel,
            this.toolStripProgressBar});
            this.statusStrip.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.HorizontalStackWithOverflow;
            this.statusStrip.Location = new System.Drawing.Point(0, 494);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(831, 22);
            this.statusStrip.TabIndex = 1;
            this.statusStrip.Text = "statusStrip1";
            // 
            // toolStripStatusLabel
            // 
            this.toolStripStatusLabel.AutoSize = false;
            this.toolStripStatusLabel.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top)
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right)
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
            this.toolStripStatusLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this.toolStripStatusLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
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
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.ouptutButtonsPanel);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 453);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(831, 41);
            this.TrafodionPanel1.TabIndex = 2;
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.helpButton.Location = new System.Drawing.Point(740, 7);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(85, 25);
            this.helpButton.TabIndex = 3;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // ExportConfigControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.ddlGroupBox);
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this.statusStrip);
            this.Name = "ExportConfigControl";
            this.Size = new System.Drawing.Size(831, 516);
            this.Load += new System.EventHandler(this.ExportConfigControl_Load);
            this.ddlGroupBox.ResumeLayout(false);
            this.ddlTextBoxPanel.ResumeLayout(false);
            this.ouptutButtonsPanel.ResumeLayout(false);
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionButton doneButton;
        private TrafodionGroupBox ddlGroupBox;
        private TrafodionPanel ouptutButtonsPanel;
        private TrafodionButton saveButton;
        private TrafodionPanel ddlTextBoxPanel;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel;
        private System.Windows.Forms.ToolStripProgressBar toolStripProgressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox ddlOutputTextBox;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionButton helpButton;
    }
}

