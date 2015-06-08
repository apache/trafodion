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
    partial class MaintainScript
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
            this.txtQueryId = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.btnSave = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnHelp = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnExecute = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.gpOption = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.rbtnOnNecessaryColumns = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.rbtnOnExistingColumn = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.rbtnOnEveryColumn = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.txtScript = new Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox();
            this.btnCopy = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.executePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.executeProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.gpOption.SuspendLayout();
            this.executePanel.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionGroupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // txtQueryId
            // 
            this.txtQueryId.BackColor = System.Drawing.Color.WhiteSmoke;
            this.txtQueryId.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtQueryId.Dock = System.Windows.Forms.DockStyle.Fill;
            this.txtQueryId.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.txtQueryId.Location = new System.Drawing.Point(10, 18);
            this.txtQueryId.Name = "txtQueryId";
            this.txtQueryId.ReadOnly = true;
            this.txtQueryId.Size = new System.Drawing.Size(746, 14);
            this.txtQueryId.TabIndex = 1;
            // 
            // btnSave
            // 
            this.btnSave.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSave.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnSave.Location = new System.Drawing.Point(531, 453);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new System.Drawing.Size(75, 23);
            this.btnSave.TabIndex = 6;
            this.btnSave.Text = "Save";
            this.btnSave.UseVisualStyleBackColor = true;
            this.btnSave.Click += new System.EventHandler(this.btnSave_Click);
            // 
            // btnHelp
            // 
            this.btnHelp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnHelp.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnHelp.Location = new System.Drawing.Point(709, 453);
            this.btnHelp.Name = "btnHelp";
            this.btnHelp.Size = new System.Drawing.Size(75, 23);
            this.btnHelp.TabIndex = 8;
            this.btnHelp.Text = "Help";
            this.btnHelp.UseVisualStyleBackColor = true;
            this.btnHelp.Click += new System.EventHandler(this.btnHelp_Click);
            // 
            // btnExecute
            // 
            this.btnExecute.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnExecute.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnExecute.Location = new System.Drawing.Point(620, 453);
            this.btnExecute.Name = "btnExecute";
            this.btnExecute.Size = new System.Drawing.Size(75, 23);
            this.btnExecute.TabIndex = 7;
            this.btnExecute.Text = "Execute";
            this.btnExecute.UseVisualStyleBackColor = true;
            this.btnExecute.Click += new System.EventHandler(this.btnExecute_Click);
            // 
            // gpOption
            // 
            this.gpOption.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.gpOption.Controls.Add(this.rbtnOnNecessaryColumns);
            this.gpOption.Controls.Add(this.rbtnOnExistingColumn);
            this.gpOption.Controls.Add(this.rbtnOnEveryColumn);
            this.gpOption.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.gpOption.Location = new System.Drawing.Point(18, 51);
            this.gpOption.Name = "gpOption";
            this.gpOption.Size = new System.Drawing.Size(766, 59);
            this.gpOption.TabIndex = 2;
            this.gpOption.TabStop = false;
            this.gpOption.Text = "Update Stats Option";
            // 
            // rbtnOnNecessaryColumns
            // 
            this.rbtnOnNecessaryColumns.AutoSize = true;
            this.rbtnOnNecessaryColumns.Checked = true;
            this.rbtnOnNecessaryColumns.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.rbtnOnNecessaryColumns.Font = new System.Drawing.Font("Tahoma", 8.5F);
            this.rbtnOnNecessaryColumns.Location = new System.Drawing.Point(10, 24);
            this.rbtnOnNecessaryColumns.Name = "rbtnOnNecessaryColumns";
            this.rbtnOnNecessaryColumns.Size = new System.Drawing.Size(176, 19);
            this.rbtnOnNecessaryColumns.TabIndex = 4;
            this.rbtnOnNecessaryColumns.TabStop = true;
            this.rbtnOnNecessaryColumns.Text = "ON NECESSARY COLUMNS";
            this.rbtnOnNecessaryColumns.UseVisualStyleBackColor = true;
            this.rbtnOnNecessaryColumns.CheckedChanged += new System.EventHandler(this.rbtnOnNecessaryColumns_CheckedChanged);
            // 
            // rbtnOnExistingColumn
            // 
            this.rbtnOnExistingColumn.AutoSize = true;
            this.rbtnOnExistingColumn.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.rbtnOnExistingColumn.Font = new System.Drawing.Font("Tahoma", 8.5F);
            this.rbtnOnExistingColumn.Location = new System.Drawing.Point(205, 24);
            this.rbtnOnExistingColumn.Name = "rbtnOnExistingColumn";
            this.rbtnOnExistingColumn.Size = new System.Drawing.Size(163, 19);
            this.rbtnOnExistingColumn.TabIndex = 3;
            this.rbtnOnExistingColumn.Text = "ON EXISTING COLUMNS";
            this.rbtnOnExistingColumn.UseVisualStyleBackColor = true;
            this.rbtnOnExistingColumn.CheckedChanged += new System.EventHandler(this.rbtnOnExistingColumn_CheckedChanged);
            // 
            // rbtnOnEveryColumn
            // 
            this.rbtnOnEveryColumn.AutoSize = true;
            this.rbtnOnEveryColumn.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.rbtnOnEveryColumn.Font = new System.Drawing.Font("Tahoma", 8.5F);
            this.rbtnOnEveryColumn.Location = new System.Drawing.Point(387, 24);
            this.rbtnOnEveryColumn.Name = "rbtnOnEveryColumn";
            this.rbtnOnEveryColumn.Size = new System.Drawing.Size(140, 19);
            this.rbtnOnEveryColumn.TabIndex = 2;
            this.rbtnOnEveryColumn.Text = "ON EVERY COLUMN";
            this.rbtnOnEveryColumn.UseVisualStyleBackColor = true;
            this.rbtnOnEveryColumn.CheckedChanged += new System.EventHandler(this.rbtnOnEveryColumn_CheckedChanged);
            // 
            // txtScript
            // 
            this.txtScript.BackColor = System.Drawing.Color.White;
            this.txtScript.Dock = System.Windows.Forms.DockStyle.Fill;
            this.txtScript.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.txtScript.ForeColor = System.Drawing.SystemColors.WindowText;
            this.txtScript.Location = new System.Drawing.Point(10, 18);
            this.txtScript.Margin = new System.Windows.Forms.Padding(10);
            this.txtScript.Name = "txtScript";
            this.txtScript.Size = new System.Drawing.Size(746, 303);
            this.txtScript.TabIndex = 5;
            this.txtScript.Text = "";
            this.txtScript.WordWrap = false;
            this.txtScript.TextChanged += new System.EventHandler(this.txtScript_TextChanged);
            // 
            // btnCopy
            // 
            this.btnCopy.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCopy.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnCopy.Location = new System.Drawing.Point(442, 453);
            this.btnCopy.Name = "btnCopy";
            this.btnCopy.Size = new System.Drawing.Size(75, 23);
            this.btnCopy.TabIndex = 6;
            this.btnCopy.Text = "Copy";
            this.btnCopy.UseVisualStyleBackColor = true;
            this.btnCopy.Click += new System.EventHandler(this.btnCopy_Click);
            // 
            // executePanel
            // 
            this.executePanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.executePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.executePanel.Controls.Add(this.executeProgressBar);
            this.executePanel.Location = new System.Drawing.Point(18, 453);
            this.executePanel.Margin = new System.Windows.Forms.Padding(0);
            this.executePanel.Name = "executePanel";
            this.executePanel.Size = new System.Drawing.Size(395, 23);
            this.executePanel.TabIndex = 9;
            // 
            // executeProgressBar
            // 
            this.executeProgressBar.Dock = System.Windows.Forms.DockStyle.Fill;
            this.executeProgressBar.Location = new System.Drawing.Point(0, 0);
            this.executeProgressBar.Margin = new System.Windows.Forms.Padding(0);
            this.executeProgressBar.Name = "executeProgressBar";
            this.executeProgressBar.Size = new System.Drawing.Size(395, 23);
            this.executeProgressBar.Step = 1;
            this.executeProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.executeProgressBar.TabIndex = 0;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionGroupBox1.Controls.Add(this.txtScript);
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(18, 113);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Padding = new System.Windows.Forms.Padding(10, 4, 10, 10);
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(766, 331);
            this.TrafodionGroupBox1.TabIndex = 10;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Command(s)";
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionGroupBox2.Controls.Add(this.txtQueryId);
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(18, 4);
            this.TrafodionGroupBox2.Margin = new System.Windows.Forms.Padding(0);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Padding = new System.Windows.Forms.Padding(10, 4, 10, 10);
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(766, 44);
            this.TrafodionGroupBox2.TabIndex = 11;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "Query ID";
            // 
            // MaintainScript
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionGroupBox2);
            this.Controls.Add(this.TrafodionGroupBox1);
            this.Controls.Add(this.executePanel);
            this.Controls.Add(this.btnCopy);
            this.Controls.Add(this.btnSave);
            this.Controls.Add(this.btnHelp);
            this.Controls.Add(this.btnExecute);
            this.Controls.Add(this.gpOption);
            this.Name = "MaintainScript";
            this.Size = new System.Drawing.Size(800, 488);
            this.gpOption.ResumeLayout(false);
            this.gpOption.PerformLayout();
            this.executePanel.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionTextBox txtQueryId;
        private Framework.Controls.TrafodionButton btnSave;
        private Framework.Controls.TrafodionButton btnHelp;
        private Framework.Controls.TrafodionButton btnExecute;
        private Framework.Controls.TrafodionGroupBox gpOption;
        private Framework.Controls.TrafodionRadioButton rbtnOnNecessaryColumns;
        private Framework.Controls.TrafodionRadioButton rbtnOnExistingColumn;
        private Framework.Controls.TrafodionRadioButton rbtnOnEveryColumn;
        private DatabaseArea.Queries.Controls.SqlStatementTextBox txtScript;
        private Framework.Controls.TrafodionButton btnCopy;
        private Framework.Controls.TrafodionPanel executePanel;
        private Framework.Controls.TrafodionProgressBar executeProgressBar;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
    }
}
