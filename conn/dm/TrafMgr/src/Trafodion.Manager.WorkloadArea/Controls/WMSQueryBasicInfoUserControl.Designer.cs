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
    partial class WMSQueryBasicInfoUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSQueryBasicInfoUserControl));
            this._theBasicQueryPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theQueryTextGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theSqlStatementTextBox = new Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox();
            this._theWarnLevelTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theWarnIndPictureBox = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.totalElapsedTimeLinkLabel = new System.Windows.Forms.LinkLabel();
            this.oneGuiLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theTotalElapsedTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSubStateTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theQueryIdTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label58 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label57 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theStateTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theBasicQueryPanel.SuspendLayout();
            this._theQueryTextGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theWarnIndPictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // _theBasicQueryPanel
            // 
            this._theBasicQueryPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theBasicQueryPanel.Controls.Add(this._theQueryTextGroupBox);
            this._theBasicQueryPanel.Controls.Add(this._theWarnLevelTextBox);
            this._theBasicQueryPanel.Controls.Add(this._theWarnIndPictureBox);
            this._theBasicQueryPanel.Controls.Add(this.totalElapsedTimeLinkLabel);
            this._theBasicQueryPanel.Controls.Add(this.oneGuiLabel2);
            this._theBasicQueryPanel.Controls.Add(this.oneGuiLabel1);
            this._theBasicQueryPanel.Controls.Add(this._theTotalElapsedTimeTextBox);
            this._theBasicQueryPanel.Controls.Add(this.label1);
            this._theBasicQueryPanel.Controls.Add(this._theSubStateTextBox);
            this._theBasicQueryPanel.Controls.Add(this._theQueryIdTextBox);
            this._theBasicQueryPanel.Controls.Add(this.label58);
            this._theBasicQueryPanel.Controls.Add(this.label57);
            this._theBasicQueryPanel.Controls.Add(this._theStateTextBox);
            this._theBasicQueryPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theBasicQueryPanel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theBasicQueryPanel.Location = new System.Drawing.Point(0, 0);
            this._theBasicQueryPanel.Name = "_theBasicQueryPanel";
            this._theBasicQueryPanel.Size = new System.Drawing.Size(833, 169);
            this._theBasicQueryPanel.TabIndex = 1;
            // 
            // _theQueryTextGroupBox
            // 
            this._theQueryTextGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theQueryTextGroupBox.Controls.Add(this._theSqlStatementTextBox);
            this._theQueryTextGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theQueryTextGroupBox.Location = new System.Drawing.Point(9, 68);
            this._theQueryTextGroupBox.Margin = new System.Windows.Forms.Padding(6, 3, 6, 3);
            this._theQueryTextGroupBox.Name = "_theQueryTextGroupBox";
            this._theQueryTextGroupBox.Padding = new System.Windows.Forms.Padding(1, 3, 1, 1);
            this._theQueryTextGroupBox.Size = new System.Drawing.Size(814, 98);
            this._theQueryTextGroupBox.TabIndex = 24;
            this._theQueryTextGroupBox.TabStop = false;
            this._theQueryTextGroupBox.Text = "Preview Text";
            // 
            // _theSqlStatementTextBox
            // 
            this._theSqlStatementTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSqlStatementTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSqlStatementTextBox.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theSqlStatementTextBox.Location = new System.Drawing.Point(1, 17);
            this._theSqlStatementTextBox.Name = "_theSqlStatementTextBox";
            this._theSqlStatementTextBox.ReadOnly = true;
            this._theSqlStatementTextBox.Size = new System.Drawing.Size(812, 80);
            this._theSqlStatementTextBox.TabIndex = 23;
            this._theSqlStatementTextBox.Text = "";
            // 
            // _theWarnLevelTextBox
            // 
            this._theWarnLevelTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theWarnLevelTextBox.Location = new System.Drawing.Point(697, 44);
            this._theWarnLevelTextBox.Name = "_theWarnLevelTextBox";
            this._theWarnLevelTextBox.ReadOnly = true;
            this._theWarnLevelTextBox.Size = new System.Drawing.Size(82, 21);
            this._theWarnLevelTextBox.TabIndex = 0;
            // 
            // _theWarnIndPictureBox
            // 
            this._theWarnIndPictureBox.Image = ((System.Drawing.Image)(resources.GetObject("_theWarnIndPictureBox.Image")));
            this._theWarnIndPictureBox.Location = new System.Drawing.Point(795, 44);
            this._theWarnIndPictureBox.Name = "_theWarnIndPictureBox";
            this._theWarnIndPictureBox.Size = new System.Drawing.Size(28, 21);
            this._theWarnIndPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
            this._theWarnIndPictureBox.TabIndex = 22;
            this._theWarnIndPictureBox.TabStop = false;
            // 
            // totalElapsedTimeLinkLabel
            // 
            this.totalElapsedTimeLinkLabel.AutoSize = true;
            this.totalElapsedTimeLinkLabel.Location = new System.Drawing.Point(467, 48);
            this.totalElapsedTimeLinkLabel.Name = "totalElapsedTimeLinkLabel";
            this.totalElapsedTimeLinkLabel.Size = new System.Drawing.Size(12, 13);
            this.totalElapsedTimeLinkLabel.TabIndex = 8;
            this.totalElapsedTimeLinkLabel.TabStop = true;
            this.totalElapsedTimeLinkLabel.Text = "?";
            // 
            // oneGuiLabel2
            // 
            this.oneGuiLabel2.AutoSize = true;
            this.oneGuiLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel2.Location = new System.Drawing.Point(629, 48);
            this.oneGuiLabel2.Name = "oneGuiLabel2";
            this.oneGuiLabel2.Size = new System.Drawing.Size(61, 13);
            this.oneGuiLabel2.TabIndex = 6;
            this.oneGuiLabel2.Text = "Warn Level";
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(371, 48);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(89, 13);
            this.oneGuiLabel1.TabIndex = 6;
            this.oneGuiLabel1.Text = "Total Query Time";
            // 
            // _theTotalElapsedTimeTextBox
            // 
            this._theTotalElapsedTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTotalElapsedTimeTextBox.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theTotalElapsedTimeTextBox.Location = new System.Drawing.Point(490, 44);
            this._theTotalElapsedTimeTextBox.Name = "_theTotalElapsedTimeTextBox";
            this._theTotalElapsedTimeTextBox.ReadOnly = true;
            this._theTotalElapsedTimeTextBox.Size = new System.Drawing.Size(127, 21);
            this._theTotalElapsedTimeTextBox.TabIndex = 7;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label1.Location = new System.Drawing.Point(6, 19);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(51, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Query ID";
            // 
            // _theSubStateTextBox
            // 
            this._theSubStateTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSubStateTextBox.Location = new System.Drawing.Point(211, 44);
            this._theSubStateTextBox.Name = "_theSubStateTextBox";
            this._theSubStateTextBox.ReadOnly = true;
            this._theSubStateTextBox.Size = new System.Drawing.Size(147, 21);
            this._theSubStateTextBox.TabIndex = 5;
            // 
            // _theQueryIdTextBox
            // 
            this._theQueryIdTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theQueryIdTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theQueryIdTextBox.Location = new System.Drawing.Point(64, 17);
            this._theQueryIdTextBox.Name = "_theQueryIdTextBox";
            this._theQueryIdTextBox.ReadOnly = true;
            this._theQueryIdTextBox.Size = new System.Drawing.Size(759, 21);
            this._theQueryIdTextBox.TabIndex = 1;
            // 
            // label58
            // 
            this.label58.AutoSize = true;
            this.label58.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label58.Location = new System.Drawing.Point(152, 48);
            this.label58.Name = "label58";
            this.label58.Size = new System.Drawing.Size(50, 13);
            this.label58.TabIndex = 4;
            this.label58.Text = "Substate";
            // 
            // label57
            // 
            this.label57.AutoSize = true;
            this.label57.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label57.Location = new System.Drawing.Point(6, 48);
            this.label57.Name = "label57";
            this.label57.Size = new System.Drawing.Size(33, 13);
            this.label57.TabIndex = 2;
            this.label57.Text = "State";
            // 
            // _theStateTextBox
            // 
            this._theStateTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theStateTextBox.Location = new System.Drawing.Point(64, 44);
            this._theStateTextBox.Name = "_theStateTextBox";
            this._theStateTextBox.ReadOnly = true;
            this._theStateTextBox.Size = new System.Drawing.Size(78, 21);
            this._theStateTextBox.TabIndex = 3;
            // 
            // WMSQueryBasicInfoUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theBasicQueryPanel);
            this.Name = "WMSQueryBasicInfoUserControl";
            this.Size = new System.Drawing.Size(833, 169);
            this._theBasicQueryPanel.ResumeLayout(false);
            this._theBasicQueryPanel.PerformLayout();
            this._theQueryTextGroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._theWarnIndPictureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theBasicQueryPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theWarnLevelTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPictureBox _theWarnIndPictureBox;
        private System.Windows.Forms.LinkLabel totalElapsedTimeLinkLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theTotalElapsedTimeTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theSubStateTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theQueryIdTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label58;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label57;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theStateTextBox;
        private Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox _theSqlStatementTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theQueryTextGroupBox;
    }
}
