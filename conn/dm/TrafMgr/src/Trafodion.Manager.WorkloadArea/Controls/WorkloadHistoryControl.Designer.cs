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
    partial class WorkloadHistoryControl
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
            this._queryPreviewTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._maxElapsedTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._minElapsedTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._avgElapsedTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._widgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._headerPanel.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _queryPreviewTextBox
            // 
            this._queryPreviewTextBox.BackColor = System.Drawing.SystemColors.Window;
            this._queryPreviewTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._queryPreviewTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._queryPreviewTextBox.Location = new System.Drawing.Point(5, 19);
            this._queryPreviewTextBox.Multiline = true;
            this._queryPreviewTextBox.Name = "_queryPreviewTextBox";
            this._queryPreviewTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this._queryPreviewTextBox.Size = new System.Drawing.Size(1033, 49);
            this._queryPreviewTextBox.TabIndex = 1;
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this.TrafodionGroupBox1);
            this._headerPanel.Controls.Add(this.TrafodionPanel1);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(1043, 107);
            this._headerPanel.TabIndex = 3;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._queryPreviewTextBox);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Padding = new System.Windows.Forms.Padding(5);
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(1043, 73);
            this.TrafodionGroupBox1.TabIndex = 2;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Query Text Preview";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._maxElapsedTimeTextBox);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel3);
            this.TrafodionPanel1.Controls.Add(this._minElapsedTimeTextBox);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel2);
            this.TrafodionPanel1.Controls.Add(this._avgElapsedTimeTextBox);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel1);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 73);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(1043, 34);
            this.TrafodionPanel1.TabIndex = 2;
            // 
            // _maxElapsedTimeTextBox
            // 
            this._maxElapsedTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._maxElapsedTimeTextBox.Location = new System.Drawing.Point(555, 8);
            this._maxElapsedTimeTextBox.Name = "_maxElapsedTimeTextBox";
            this._maxElapsedTimeTextBox.ReadOnly = true;
            this._maxElapsedTimeTextBox.Size = new System.Drawing.Size(100, 21);
            this._maxElapsedTimeTextBox.TabIndex = 1;
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(451, 11);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(99, 13);
            this.TrafodionLabel3.TabIndex = 0;
            this.TrafodionLabel3.Text = "Max. Elapsed Time ";
            // 
            // _minElapsedTimeTextBox
            // 
            this._minElapsedTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._minElapsedTimeTextBox.Location = new System.Drawing.Point(332, 8);
            this._minElapsedTimeTextBox.Name = "_minElapsedTimeTextBox";
            this._minElapsedTimeTextBox.ReadOnly = true;
            this._minElapsedTimeTextBox.Size = new System.Drawing.Size(100, 21);
            this._minElapsedTimeTextBox.TabIndex = 1;
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(228, 11);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(95, 13);
            this.TrafodionLabel2.TabIndex = 0;
            this.TrafodionLabel2.Text = "Min. Elapsed Time ";
            // 
            // _avgElapsedTimeTextBox
            // 
            this._avgElapsedTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._avgElapsedTimeTextBox.Location = new System.Drawing.Point(113, 8);
            this._avgElapsedTimeTextBox.Name = "_avgElapsedTimeTextBox";
            this._avgElapsedTimeTextBox.ReadOnly = true;
            this._avgElapsedTimeTextBox.Size = new System.Drawing.Size(100, 21);
            this._avgElapsedTimeTextBox.TabIndex = 1;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(9, 11);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(98, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Avg. Elapsed Time ";
            // 
            // _widgetPanel
            // 
            this._widgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._widgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._widgetPanel.Location = new System.Drawing.Point(0, 107);
            this._widgetPanel.Name = "_widgetPanel";
            this._widgetPanel.Size = new System.Drawing.Size(1043, 494);
            this._widgetPanel.TabIndex = 4;
            // 
            // WorkloadHistoryControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._widgetPanel);
            this.Controls.Add(this._headerPanel);
            this.Name = "WorkloadHistoryControl";
            this.Size = new System.Drawing.Size(1043, 601);
            this._headerPanel.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionTextBox _queryPreviewTextBox;
        private Framework.Controls.TrafodionPanel _headerPanel;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Framework.Controls.TrafodionPanel _widgetPanel;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionTextBox _maxElapsedTimeTextBox;
        private Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Framework.Controls.TrafodionTextBox _minElapsedTimeTextBox;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionTextBox _avgElapsedTimeTextBox;
        private Framework.Controls.TrafodionLabel TrafodionLabel1;
    }
}
