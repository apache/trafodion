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
﻿namespace Trafodion.Manager.UserManagement.Controls
{
    partial class DropDialog
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
            this.btnNo = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnYes = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.grbGrantorSelection = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.lblCascade = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.chkCascade = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.lblWarning = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.grbGrantorSelection.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnNo
            // 
            this.btnNo.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnNo.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnNo.Location = new System.Drawing.Point(327, 115);
            this.btnNo.Name = "btnNo";
            this.btnNo.Size = new System.Drawing.Size(62, 25);
            this.btnNo.TabIndex = 6;
            this.btnNo.Text = "&No";
            this.btnNo.UseVisualStyleBackColor = true;
            this.btnNo.Click += new System.EventHandler(this.btnNo_Click);
            // 
            // btnYes
            // 
            this.btnYes.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnYes.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.btnYes.Location = new System.Drawing.Point(254, 115);
            this.btnYes.Name = "btnYes";
            this.btnYes.Size = new System.Drawing.Size(62, 25);
            this.btnYes.TabIndex = 5;
            this.btnYes.Text = "&Yes";
            this.btnYes.UseVisualStyleBackColor = true;
            this.btnYes.Click += new System.EventHandler(this.btnYes_Click);
            // 
            // grbGrantorSelection
            // 
            this.grbGrantorSelection.Controls.Add(this.lblCascade);
            this.grbGrantorSelection.Controls.Add(this.chkCascade);
            this.grbGrantorSelection.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.grbGrantorSelection.Location = new System.Drawing.Point(12, 12);
            this.grbGrantorSelection.Name = "grbGrantorSelection";
            this.grbGrantorSelection.Size = new System.Drawing.Size(377, 90);
            this.grbGrantorSelection.TabIndex = 7;
            this.grbGrantorSelection.TabStop = false;
            // 
            // lblCascade
            // 
            this.lblCascade.AutoSize = true;
            this.lblCascade.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblCascade.Location = new System.Drawing.Point(13, 27);
            this.lblCascade.Margin = new System.Windows.Forms.Padding(3);
            this.lblCascade.Name = "lblCascade";
            this.lblCascade.Size = new System.Drawing.Size(226, 39);
            this.lblCascade.TabIndex = 3;
            this.lblCascade.Text = "Select Cascade \r\nto revoke each role\'s SQL privileges \r\nand revoke the role(s) fr" +
    "om all granted users:";
            // 
            // chkCascade
            // 
            this.chkCascade.AutoSize = true;
            this.chkCascade.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.chkCascade.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.chkCascade.Location = new System.Drawing.Point(271, 37);
            this.chkCascade.Name = "chkCascade";
            this.chkCascade.Size = new System.Drawing.Size(73, 18);
            this.chkCascade.TabIndex = 2;
            this.chkCascade.Text = "Cascade";
            this.chkCascade.UseVisualStyleBackColor = true;
            // 
            // lblWarning
            // 
            this.lblWarning.AutoSize = true;
            this.lblWarning.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblWarning.Location = new System.Drawing.Point(25, 121);
            this.lblWarning.Margin = new System.Windows.Forms.Padding(3);
            this.lblWarning.Name = "lblWarning";
            this.lblWarning.Size = new System.Drawing.Size(203, 13);
            this.lblWarning.TabIndex = 1;
            this.lblWarning.Text = "Are you sure you want to drop role(s)?";
            // 
            // DropDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(401, 149);
            this.Controls.Add(this.grbGrantorSelection);
            this.Controls.Add(this.btnNo);
            this.Controls.Add(this.lblWarning);
            this.Controls.Add(this.btnYes);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DropDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Drop Option";
            this.grbGrantorSelection.ResumeLayout(false);
            this.grbGrantorSelection.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionButton btnNo;
        private Framework.Controls.TrafodionButton btnYes;
        private Framework.Controls.TrafodionGroupBox grbGrantorSelection;
        private Framework.Controls.TrafodionLabel lblWarning;
        private Framework.Controls.TrafodionLabel lblCascade;
        private Framework.Controls.TrafodionCheckBox chkCascade;

    }
}