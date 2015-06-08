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
    partial class ParentChildQueriesUserControl
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
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._queryIDTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._queryIDLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._planGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._parentChildQueriesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._headerPanel.SuspendLayout();
            this._planGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._queryIDTextBox);
            this._headerPanel.Controls.Add(this._queryIDLabel);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(1001, 36);
            this._headerPanel.TabIndex = 2;
            // 
            // _queryIDTextBox
            // 
            this._queryIDTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._queryIDTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._queryIDTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._queryIDTextBox.Location = new System.Drawing.Point(68, 6);
            this._queryIDTextBox.Name = "_queryIDTextBox";
            this._queryIDTextBox.ReadOnly = true;
            this._queryIDTextBox.Size = new System.Drawing.Size(923, 21);
            this._queryIDTextBox.TabIndex = 1;
            // 
            // _queryIDLabel
            // 
            this._queryIDLabel.AutoSize = true;
            this._queryIDLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._queryIDLabel.Location = new System.Drawing.Point(10, 9);
            this._queryIDLabel.Name = "_queryIDLabel";
            this._queryIDLabel.Size = new System.Drawing.Size(57, 13);
            this._queryIDLabel.TabIndex = 0;
            this._queryIDLabel.Text = "Query ID";
            // 
            // _planGroupBox
            // 
            this._planGroupBox.Controls.Add(this._parentChildQueriesPanel);
            this._planGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._planGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._planGroupBox.Location = new System.Drawing.Point(0, 36);
            this._planGroupBox.Name = "_planGroupBox";
            this._planGroupBox.Padding = new System.Windows.Forms.Padding(5);
            this._planGroupBox.Size = new System.Drawing.Size(1001, 629);
            this._planGroupBox.TabIndex = 3;
            this._planGroupBox.TabStop = false;
            this._planGroupBox.Text = "Parent/Child Queries";
            // 
            // _parentChildQueriesPanel
            // 
            this._parentChildQueriesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._parentChildQueriesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._parentChildQueriesPanel.Location = new System.Drawing.Point(5, 19);
            this._parentChildQueriesPanel.Name = "_parentChildQueriesPanel";
            this._parentChildQueriesPanel.Size = new System.Drawing.Size(991, 605);
            this._parentChildQueriesPanel.TabIndex = 2;
            // 
            // ParentChildQueriesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._planGroupBox);
            this.Controls.Add(this._headerPanel);
            this.Name = "ParentChildQueriesUserControl";
            this.Size = new System.Drawing.Size(1001, 665);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this._planGroupBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _headerPanel;
        private Framework.Controls.TrafodionTextBox _queryIDTextBox;
        private Framework.Controls.TrafodionLabel _queryIDLabel;
        private Framework.Controls.TrafodionGroupBox _planGroupBox;
        private Framework.Controls.TrafodionPanel _parentChildQueriesPanel;
    }
}
