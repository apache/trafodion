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
    partial class TablePrivilegesOptionControl
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
            this._objectLevelPrivilegesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._referenceCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._updateCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._selectCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._insertCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._deleteCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._colPrivTableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._allCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionToolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._columnLevelPrivilegesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._replicateCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._objectLevelPrivilegesGroupBox.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this._columnLevelPrivilegesGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _objectLevelPrivilegesGroupBox
            // 
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._referenceCheckBox);
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._updateCheckBox);
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._selectCheckBox);
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._insertCheckBox);
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._replicateCheckBox);
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._deleteCheckBox);
            this._objectLevelPrivilegesGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._objectLevelPrivilegesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._objectLevelPrivilegesGroupBox.Location = new System.Drawing.Point(0, 39);
            this._objectLevelPrivilegesGroupBox.Name = "_objectLevelPrivilegesGroupBox";
            this._objectLevelPrivilegesGroupBox.Size = new System.Drawing.Size(636, 55);
            this._objectLevelPrivilegesGroupBox.TabIndex = 5;
            this._objectLevelPrivilegesGroupBox.TabStop = false;
            this._objectLevelPrivilegesGroupBox.Text = "Object Level Privileges";
            // 
            // _referenceCheckBox
            // 
            this._referenceCheckBox.AutoSize = true;
            this._referenceCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._referenceCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._referenceCheckBox.Location = new System.Drawing.Point(267, 22);
            this._referenceCheckBox.Name = "_referenceCheckBox";
            this._referenceCheckBox.Size = new System.Drawing.Size(82, 18);
            this._referenceCheckBox.TabIndex = 7;
            this._referenceCheckBox.Text = "Reference";
            this._referenceCheckBox.UseVisualStyleBackColor = true;
            this._referenceCheckBox.CheckedChanged += new System.EventHandler(this._referenceCheckBox_CheckedChanged);
            // 
            // _updateCheckBox
            // 
            this._updateCheckBox.AutoSize = true;
            this._updateCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._updateCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._updateCheckBox.Location = new System.Drawing.Point(178, 22);
            this._updateCheckBox.Name = "_updateCheckBox";
            this._updateCheckBox.Size = new System.Drawing.Size(67, 18);
            this._updateCheckBox.TabIndex = 6;
            this._updateCheckBox.Text = "Update";
            this._updateCheckBox.UseVisualStyleBackColor = true;
            this._updateCheckBox.CheckedChanged += new System.EventHandler(this._updateCheckBox_CheckedChanged);
            // 
            // _selectCheckBox
            // 
            this._selectCheckBox.AutoSize = true;
            this._selectCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._selectCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._selectCheckBox.Location = new System.Drawing.Point(12, 22);
            this._selectCheckBox.Name = "_selectCheckBox";
            this._selectCheckBox.Size = new System.Drawing.Size(61, 18);
            this._selectCheckBox.TabIndex = 5;
            this._selectCheckBox.Text = "Select";
            this._selectCheckBox.UseVisualStyleBackColor = true;
            this._selectCheckBox.CheckedChanged += new System.EventHandler(this._selectCheckBox_CheckedChanged);
            // 
            // _insertCheckBox
            // 
            this._insertCheckBox.AutoSize = true;
            this._insertCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._insertCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._insertCheckBox.Location = new System.Drawing.Point(95, 22);
            this._insertCheckBox.Name = "_insertCheckBox";
            this._insertCheckBox.Size = new System.Drawing.Size(61, 18);
            this._insertCheckBox.TabIndex = 4;
            this._insertCheckBox.Text = "Insert";
            this._insertCheckBox.UseVisualStyleBackColor = true;
            this._insertCheckBox.CheckedChanged += new System.EventHandler(this._insertCheckBox_CheckedChanged);
            // 
            // _deleteCheckBox
            // 
            this._deleteCheckBox.AutoSize = true;
            this._deleteCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._deleteCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._deleteCheckBox.Location = new System.Drawing.Point(371, 22);
            this._deleteCheckBox.Name = "_deleteCheckBox";
            this._deleteCheckBox.Size = new System.Drawing.Size(63, 18);
            this._deleteCheckBox.TabIndex = 3;
            this._deleteCheckBox.Text = "Delete";
            this._deleteCheckBox.UseVisualStyleBackColor = true;
            // 
            // _colPrivTableLayoutPanel
            // 
            this._colPrivTableLayoutPanel.AutoScroll = true;
            this._colPrivTableLayoutPanel.ColumnCount = 4;
            this._colPrivTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this._colPrivTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this._colPrivTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this._colPrivTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 25F));
            this._colPrivTableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._colPrivTableLayoutPanel.Location = new System.Drawing.Point(3, 17);
            this._colPrivTableLayoutPanel.Name = "_colPrivTableLayoutPanel";
            this._colPrivTableLayoutPanel.RowCount = 1;
            this._colPrivTableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this._colPrivTableLayoutPanel.Size = new System.Drawing.Size(630, 410);
            this._colPrivTableLayoutPanel.TabIndex = 8;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._allCheckBox);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(636, 39);
            this.TrafodionPanel1.TabIndex = 9;
            // 
            // _allCheckBox
            // 
            this._allCheckBox.AutoSize = true;
            this._allCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._allCheckBox.Location = new System.Drawing.Point(12, 8);
            this._allCheckBox.Name = "_allCheckBox";
            this._allCheckBox.Size = new System.Drawing.Size(49, 18);
            this._allCheckBox.TabIndex = 4;
            this._allCheckBox.Text = "ALL";
            this._allCheckBox.UseVisualStyleBackColor = true;
            this._allCheckBox.CheckedChanged += new System.EventHandler(this._allCheckBox_CheckedChanged);
            // 
            // TrafodionToolTip1
            // 
            this.TrafodionToolTip1.IsBalloon = true;
            // 
            // _columnLevelPrivilegesGroupBox
            // 
            this._columnLevelPrivilegesGroupBox.Controls.Add(this._colPrivTableLayoutPanel);
            this._columnLevelPrivilegesGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._columnLevelPrivilegesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._columnLevelPrivilegesGroupBox.Location = new System.Drawing.Point(0, 94);
            this._columnLevelPrivilegesGroupBox.Name = "_columnLevelPrivilegesGroupBox";
            this._columnLevelPrivilegesGroupBox.Size = new System.Drawing.Size(636, 430);
            this._columnLevelPrivilegesGroupBox.TabIndex = 10;
            this._columnLevelPrivilegesGroupBox.TabStop = false;
            this._columnLevelPrivilegesGroupBox.Text = "Column Level Privileges";
            // 
            // _replicateCheckBox
            // 
            this._replicateCheckBox.AutoSize = true;
            this._replicateCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._replicateCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._replicateCheckBox.Location = new System.Drawing.Point(454, 22);
            this._replicateCheckBox.Name = "_replicateCheckBox";
            this._replicateCheckBox.Size = new System.Drawing.Size(76, 18);
            this._replicateCheckBox.TabIndex = 3;
            this._replicateCheckBox.Text = "Replicate";
            this._replicateCheckBox.UseVisualStyleBackColor = true;
            // 
            // TablePrivilegesOptionControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.Controls.Add(this._columnLevelPrivilegesGroupBox);
            this.Controls.Add(this._objectLevelPrivilegesGroupBox);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "TablePrivilegesOptionControl";
            this.Size = new System.Drawing.Size(636, 524);
            this._objectLevelPrivilegesGroupBox.ResumeLayout(false);
            this._objectLevelPrivilegesGroupBox.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this._columnLevelPrivilegesGroupBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _objectLevelPrivilegesGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _deleteCheckBox;
        private System.Windows.Forms.TableLayoutPanel _colPrivTableLayoutPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionToolTip TrafodionToolTip1;
        private Framework.Controls.TrafodionCheckBox _selectCheckBox;
        private Framework.Controls.TrafodionCheckBox _insertCheckBox;
        private Framework.Controls.TrafodionCheckBox _referenceCheckBox;
        private Framework.Controls.TrafodionCheckBox _updateCheckBox;
        private Framework.Controls.TrafodionCheckBox _allCheckBox;
        private Framework.Controls.TrafodionGroupBox _columnLevelPrivilegesGroupBox;
        private Framework.Controls.TrafodionCheckBox _replicateCheckBox;
    }
}
