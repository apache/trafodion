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
    partial class GrantorSelectionDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(GrantorSelectionDialog));
            this.grbGrantorSelection = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblGrantedBy = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.rbtnRole = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.rbtnUser = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.btnCancel = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnOK = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.grdGrantor = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.grbGrantorSelection.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.grdGrantor)).BeginInit();
            this.SuspendLayout();
            // 
            // grbGrantorSelection
            // 
            this.grbGrantorSelection.Controls.Add(this.lblGrantedBy);
            this.grbGrantorSelection.Controls.Add(this.rbtnRole);
            this.grbGrantorSelection.Controls.Add(this.rbtnUser);
            this.grbGrantorSelection.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.grbGrantorSelection.Location = new System.Drawing.Point(10, 9);
            this.grbGrantorSelection.Name = "grbGrantorSelection";
            this.grbGrantorSelection.Size = new System.Drawing.Size(321, 42);
            this.grbGrantorSelection.TabIndex = 1;
            this.grbGrantorSelection.TabStop = false;
            // 
            // lblGrantedBy
            // 
            this.lblGrantedBy.AutoSize = true;
            this.lblGrantedBy.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblGrantedBy.Location = new System.Drawing.Point(4, 18);
            this.lblGrantedBy.Margin = new System.Windows.Forms.Padding(3);
            this.lblGrantedBy.Name = "lblGrantedBy";
            this.lblGrantedBy.Size = new System.Drawing.Size(113, 13);
            this.lblGrantedBy.TabIndex = 1;
            this.lblGrantedBy.Text = "Select a grantor from:";
            // 
            // rbtnRole
            // 
            this.rbtnRole.AutoSize = true;
            this.rbtnRole.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.rbtnRole.Location = new System.Drawing.Point(221, 16);
            this.rbtnRole.Name = "rbtnRole";
            this.rbtnRole.Size = new System.Drawing.Size(89, 18);
            this.rbtnRole.TabIndex = 0;
            this.rbtnRole.Text = "These Roles";
            this.rbtnRole.UseVisualStyleBackColor = true;
            this.rbtnRole.CheckedChanged += new System.EventHandler(this.rbtnRole_CheckedChanged);
            // 
            // rbtnUser
            // 
            this.rbtnUser.AutoSize = true;
            this.rbtnUser.Checked = true;
            this.rbtnUser.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.rbtnUser.Location = new System.Drawing.Point(128, 16);
            this.rbtnUser.Name = "rbtnUser";
            this.rbtnUser.Size = new System.Drawing.Size(90, 18);
            this.rbtnUser.TabIndex = 0;
            this.rbtnUser.TabStop = true;
            this.rbtnUser.Text = "These Users";
            this.rbtnUser.UseVisualStyleBackColor = true;
            this.rbtnUser.CheckedChanged += new System.EventHandler(this.rbtnUser_CheckedChanged);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnCancel.Location = new System.Drawing.Point(256, 317);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 25);
            this.btnCancel.TabIndex = 4;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Enabled = false;
            this.btnOK.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnOK.Location = new System.Drawing.Point(172, 317);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 25);
            this.btnOK.TabIndex = 3;
            this.btnOK.Text = "&OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // grdGrantor
            // 
            this.grdGrantor.AllowColumnFilter = true;
            this.grdGrantor.AllowWordWrap = false;
            this.grdGrantor.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("grdGrantor.AlwaysHiddenColumnNames")));
            this.grdGrantor.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.grdGrantor.CurrentFilter = null;
            this.grdGrantor.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.grdGrantor.ForeColor = System.Drawing.SystemColors.WindowText;
            this.grdGrantor.Header.Height = 20;
            this.grdGrantor.HelpTopic = "";
            this.grdGrantor.Location = new System.Drawing.Point(10, 61);
            this.grdGrantor.Margin = new System.Windows.Forms.Padding(0);
            this.grdGrantor.Name = "grdGrantor";
            this.grdGrantor.ReadOnly = true;
            this.grdGrantor.RowMode = true;
            this.grdGrantor.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.grdGrantor.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.grdGrantor.SearchAsType.SearchCol = null;
            this.grdGrantor.Size = new System.Drawing.Size(321, 250);
            this.grdGrantor.TabIndex = 1;
            this.grdGrantor.TreeCol = null;
            this.grdGrantor.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.grdGrantor.WordWrap = false;
            // 
            // GrantorSelectionDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(343, 348);
            this.Controls.Add(this.grdGrantor);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.grbGrantorSelection);
            this.Controls.Add(this.btnOK);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "GrantorSelectionDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "HP Database Manager - Grantor Dialog";
            this.grbGrantorSelection.ResumeLayout(false);
            this.grbGrantorSelection.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.grdGrantor)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox grbGrantorSelection;
        private Framework.Controls.TrafodionRadioButton rbtnRole;
        private Framework.Controls.TrafodionRadioButton rbtnUser;
        private Framework.Controls.TrafodionButton btnCancel;
        private Framework.Controls.TrafodionButton btnOK;
        private Framework.Controls.TrafodionLabel lblGrantedBy;
        private Framework.Controls.TrafodionIGrid grdGrantor;
    }
}