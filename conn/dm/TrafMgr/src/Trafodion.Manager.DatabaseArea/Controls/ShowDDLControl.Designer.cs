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

namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class ShowDDLControl : UserControl
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
            this.splitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.selectionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.treePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.ddlGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.ddlOutputTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.clearButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.loadButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.appendButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.clearTextButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.saveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.doneButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.splitContainer.Panel1.SuspendLayout();
            this.splitContainer.Panel2.SuspendLayout();
            this.splitContainer.SuspendLayout();
            this.selectionGroupBox.SuspendLayout();
            this.ddlGroupBox.SuspendLayout();
            this.statusStrip.SuspendLayout();
            this._buttonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // splitContainer
            // 
            this.splitContainer.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.splitContainer.BackColor = System.Drawing.Color.WhiteSmoke;
            this.splitContainer.Location = new System.Drawing.Point(0, 0);
            this.splitContainer.Name = "splitContainer";
            // 
            // splitContainer.Panel1
            // 
            this.splitContainer.Panel1.AllowDrop = true;
            this.splitContainer.Panel1.Controls.Add(this.selectionGroupBox);
            // 
            // splitContainer.Panel2
            // 
            this.splitContainer.Panel2.Controls.Add(this.ddlGroupBox);
            this.splitContainer.Size = new System.Drawing.Size(933, 640);
            this.splitContainer.SplitterDistance = 385;
            this.splitContainer.SplitterWidth = 9;
            this.splitContainer.TabIndex = 0;
            // 
            // selectionGroupBox
            // 
            this.selectionGroupBox.Controls.Add(this.treePanel);
            this.selectionGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.selectionGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.selectionGroupBox.Location = new System.Drawing.Point(0, 0);
            this.selectionGroupBox.Name = "selectionGroupBox";
            this.selectionGroupBox.Size = new System.Drawing.Size(385, 640);
            this.selectionGroupBox.TabIndex = 2;
            this.selectionGroupBox.TabStop = false;
            this.selectionGroupBox.Text = "Select Objects";
            // 
            // treePanel
            // 
            this.treePanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(255)))), ((int)(((byte)(255)))));
            this.treePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treePanel.Location = new System.Drawing.Point(3, 17);
            this.treePanel.Name = "treePanel";
            this.treePanel.Size = new System.Drawing.Size(379, 620);
            this.treePanel.TabIndex = 0;
            // 
            // ddlGroupBox
            // 
            this.ddlGroupBox.AutoSize = true;
            this.ddlGroupBox.Controls.Add(this.ddlOutputTextBox);
            this.ddlGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ddlGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ddlGroupBox.Location = new System.Drawing.Point(0, 0);
            this.ddlGroupBox.Name = "ddlGroupBox";
            this.ddlGroupBox.Size = new System.Drawing.Size(539, 640);
            this.ddlGroupBox.TabIndex = 2;
            this.ddlGroupBox.TabStop = false;
            this.ddlGroupBox.Text = "DDL Output";
            // 
            // ddlOutputTextBox
            // 
            this.ddlOutputTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ddlOutputTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ddlOutputTextBox.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ddlOutputTextBox.Location = new System.Drawing.Point(3, 17);
            this.ddlOutputTextBox.Name = "ddlOutputTextBox";
            this.ddlOutputTextBox.ReadOnly = true;
            this.ddlOutputTextBox.Size = new System.Drawing.Size(533, 620);
            this.ddlOutputTextBox.TabIndex = 0;
            this.ddlOutputTextBox.Text = "";
            this.ddlOutputTextBox.WordWrap = false;
            // 
            // clearButton
            // 
            this.clearButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.clearButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.clearButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.clearButton.Location = new System.Drawing.Point(171, 7);
            this.clearButton.Name = "clearButton";
            this.clearButton.Size = new System.Drawing.Size(75, 23);
            this.clearButton.TabIndex = 1;
            this.clearButton.Text = "Cl&ear All";
            this.clearButton.UseVisualStyleBackColor = true;
            this.clearButton.Click += new System.EventHandler(this.clearButton_Click);
            // 
            // loadButton
            // 
            this.loadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.loadButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.loadButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.loadButton.Location = new System.Drawing.Point(3, 7);
            this.loadButton.Name = "loadButton";
            this.loadButton.Size = new System.Drawing.Size(75, 23);
            this.loadButton.TabIndex = 1;
            this.loadButton.Text = "&Load";
            this.loadButton.UseVisualStyleBackColor = true;
            this.loadButton.Click += new System.EventHandler(this.loadButton_Click);
            // 
            // appendButton
            // 
            this.appendButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.appendButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this.appendButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.appendButton.Location = new System.Drawing.Point(87, 7);
            this.appendButton.Name = "appendButton";
            this.appendButton.Size = new System.Drawing.Size(75, 23);
            this.appendButton.TabIndex = 1;
            this.appendButton.Text = "&Append";
            this.appendButton.UseVisualStyleBackColor = true;
            this.appendButton.Click += new System.EventHandler(this.appendButton_Click);
            // 
            // clearTextButton
            // 
            this.clearTextButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.clearTextButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.clearTextButton.Location = new System.Drawing.Point(676, 7);
            this.clearTextButton.Name = "clearTextButton";
            this.clearTextButton.Size = new System.Drawing.Size(75, 23);
            this.clearTextButton.TabIndex = 2;
            this.clearTextButton.Text = "Clea&r";
            this.clearTextButton.UseVisualStyleBackColor = true;
            this.clearTextButton.Click += new System.EventHandler(this.clearTextButton_Click);
            // 
            // saveButton
            // 
            this.saveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.saveButton.Enabled = false;
            this.saveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.saveButton.Location = new System.Drawing.Point(763, 7);
            this.saveButton.Name = "saveButton";
            this.saveButton.Size = new System.Drawing.Size(75, 23);
            this.saveButton.TabIndex = 2;
            this.saveButton.Text = "&Save";
            this.saveButton.UseVisualStyleBackColor = true;
            this.saveButton.Click += new System.EventHandler(this.saveButton_Click);
            // 
            // doneButton
            // 
            this.doneButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.doneButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.doneButton.Location = new System.Drawing.Point(850, 7);
            this.doneButton.Name = "doneButton";
            this.doneButton.Size = new System.Drawing.Size(75, 23);
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
            this.statusStrip.Location = new System.Drawing.Point(0, 684);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(933, 22);
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
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._buttonsPanel.Controls.Add(this.clearTextButton);
            this._buttonsPanel.Controls.Add(this.saveButton);
            this._buttonsPanel.Controls.Add(this.clearButton);
            this._buttonsPanel.Controls.Add(this.doneButton);
            this._buttonsPanel.Controls.Add(this.loadButton);
            this._buttonsPanel.Controls.Add(this.appendButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 646);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(933, 38);
            this._buttonsPanel.TabIndex = 2;
            // 
            // ShowDDLControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._buttonsPanel);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.splitContainer);
            this.Name = "ShowDDLControl";
            this.Size = new System.Drawing.Size(933, 706);
            this.Load += new System.EventHandler(this.ShowDDLControl_Load);
            this.splitContainer.Panel1.ResumeLayout(false);
            this.splitContainer.Panel2.ResumeLayout(false);
            this.splitContainer.Panel2.PerformLayout();
            this.splitContainer.ResumeLayout(false);
            this.selectionGroupBox.ResumeLayout(false);
            this.ddlGroupBox.ResumeLayout(false);
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this._buttonsPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionSplitContainer splitContainer;
        private TrafodionButton doneButton;
        private TrafodionGroupBox selectionGroupBox;
        private TrafodionGroupBox ddlGroupBox;
        private TrafodionButton saveButton;
        private TrafodionButton appendButton;
        private TrafodionButton loadButton;
        private TrafodionButton clearButton;
        private TrafodionPanel treePanel;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel;
        private System.Windows.Forms.ToolStripProgressBar toolStripProgressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox ddlOutputTextBox;
        private TrafodionButton clearTextButton;
        private TrafodionPanel _buttonsPanel;
    }
}

