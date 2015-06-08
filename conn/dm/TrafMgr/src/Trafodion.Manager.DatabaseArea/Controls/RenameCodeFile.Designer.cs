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
    partial class RenameCodeFile
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
            this._theCodeFileName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theRenameButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._errorLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.SuspendLayout();
            // 
            // _theCodeFileName
            // 
            this._theCodeFileName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theCodeFileName.Location = new System.Drawing.Point(81, 30);
            this._theCodeFileName.Name = "_theCodeFileName";
            this._theCodeFileName.Size = new System.Drawing.Size(304, 20);
            this._theCodeFileName.TabIndex = 1;
            this._theCodeFileName.TextChanged += new System.EventHandler(this._theCodeFileName_TextChanged);
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(12, 33);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(63, 14);
            this.oneGuiLabel1.TabIndex = 0;
            this.oneGuiLabel1.Text = "New Name:";
            this.oneGuiLabel1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theRenameButton
            // 
            this._theRenameButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theRenameButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._theRenameButton.Enabled = false;
            this._theRenameButton.Location = new System.Drawing.Point(226, 65);
            this._theRenameButton.Name = "_theRenameButton";
            this._theRenameButton.Size = new System.Drawing.Size(75, 23);
            this._theRenameButton.TabIndex = 4;
            this._theRenameButton.Text = "O&K";
            this._theRenameButton.UseVisualStyleBackColor = true;
            this._theRenameButton.Click += new System.EventHandler(this._theCreateButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._theCancelButton.Location = new System.Drawing.Point(310, 65);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 5;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _errorLabel
            // 
            this._errorLabel.AutoSize = true;
            this._errorLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._errorLabel.ForeColor = System.Drawing.Color.Red;
            this._errorLabel.Location = new System.Drawing.Point(12, 0);
            this._errorLabel.Name = "_errorLabel";
            this._errorLabel.Size = new System.Drawing.Size(0, 14);
            this._errorLabel.TabIndex = 0;
            this._errorLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // RenameCodeFile
            // 
            this.AcceptButton = this._theRenameButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CancelButton = this._theCancelButton;
            this.ClientSize = new System.Drawing.Size(402, 100);
            this.Controls.Add(this._theCancelButton);
            this.Controls.Add(this._theRenameButton);
            this.Controls.Add(this._theCodeFileName);
            this.Controls.Add(this._errorLabel);
            this.Controls.Add(this.oneGuiLabel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Name = "RenameCodeFile";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "HP Database Manager - Rename Code File";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theCodeFileName;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theRenameButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _errorLabel;
    }
}
