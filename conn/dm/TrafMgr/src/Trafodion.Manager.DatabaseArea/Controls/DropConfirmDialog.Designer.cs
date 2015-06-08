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
    partial class DropConfirmDialog
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
            this._theMessageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._thePicBox = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this._theCheckOption = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theYesTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theNoTrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theNoteLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            ((System.ComponentModel.ISupportInitialize)(this._thePicBox)).BeginInit();
            this.SuspendLayout();
            // 
            // _theMessageLabel
            // 
            this._theMessageLabel.AutoSize = true;
            this._theMessageLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theMessageLabel.Location = new System.Drawing.Point(68, 12);
            this._theMessageLabel.Name = "_theMessageLabel";
            this._theMessageLabel.Size = new System.Drawing.Size(71, 13);
            this._theMessageLabel.TabIndex = 1;
            this._theMessageLabel.Text = "TrafodionLabel1";
            // 
            // _thePicBox
            // 
            this._thePicBox.Location = new System.Drawing.Point(12, 12);
            this._thePicBox.Name = "_thePicBox";
            this._thePicBox.Size = new System.Drawing.Size(40, 40);
            this._thePicBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this._thePicBox.TabIndex = 2;
            this._thePicBox.TabStop = false;
            // 
            // _theCheckOption
            // 
            this._theCheckOption.AutoSize = true;
            this._theCheckOption.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCheckOption.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCheckOption.Location = new System.Drawing.Point(71, 33);
            this._theCheckOption.Name = "_theCheckOption";
            this._theCheckOption.Size = new System.Drawing.Size(93, 18);
            this._theCheckOption.TabIndex = 3;
            this._theCheckOption.Text = "CheckOption";
            this._theCheckOption.UseVisualStyleBackColor = true;
            this._theCheckOption.CheckedChanged += new System.EventHandler(this._theCheckOption_CheckedChanged);
            // 
            // _theYesTrafodionButton
            // 
            this._theYesTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theYesTrafodionButton.DialogResult = System.Windows.Forms.DialogResult.Yes;
            this._theYesTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theYesTrafodionButton.Location = new System.Drawing.Point(257, 103);
            this._theYesTrafodionButton.Name = "_theYesTrafodionButton";
            this._theYesTrafodionButton.Size = new System.Drawing.Size(75, 23);
            this._theYesTrafodionButton.TabIndex = 4;
            this._theYesTrafodionButton.Text = "&Yes";
            this._theYesTrafodionButton.UseVisualStyleBackColor = true;
            // 
            // _theNoTrafodionButton
            // 
            this._theNoTrafodionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theNoTrafodionButton.DialogResult = System.Windows.Forms.DialogResult.No;
            this._theNoTrafodionButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theNoTrafodionButton.Location = new System.Drawing.Point(338, 103);
            this._theNoTrafodionButton.Name = "_theNoTrafodionButton";
            this._theNoTrafodionButton.Size = new System.Drawing.Size(75, 23);
            this._theNoTrafodionButton.TabIndex = 4;
            this._theNoTrafodionButton.Text = "&No";
            this._theNoTrafodionButton.UseVisualStyleBackColor = true;
            // 
            // _theNoteLabel
            // 
            this._theNoteLabel.AutoSize = true;
            this._theNoteLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theNoteLabel.Location = new System.Drawing.Point(68, 63);
            this._theNoteLabel.MaximumSize = new System.Drawing.Size(360, 0);
            this._theNoteLabel.Name = "_theNoteLabel";
            this._theNoteLabel.Size = new System.Drawing.Size(0, 13);
            this._theNoteLabel.TabIndex = 5;
            // 
            // DropConfirmDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(424, 137);
            this.Controls.Add(this._theNoteLabel);
            this.Controls.Add(this._theNoTrafodionButton);
            this.Controls.Add(this._theYesTrafodionButton);
            this.Controls.Add(this._theCheckOption);
            this.Controls.Add(this._thePicBox);
            this.Controls.Add(this._theMessageLabel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DropConfirmDialog";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - DropConfirmDialog";
            ((System.ComponentModel.ISupportInitialize)(this._thePicBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionLabel _theMessageLabel;
        private Framework.Controls.TrafodionPictureBox _thePicBox;
        private Framework.Controls.TrafodionCheckBox _theCheckOption;
        private Framework.Controls.TrafodionButton _theYesTrafodionButton;
        private Framework.Controls.TrafodionButton _theNoTrafodionButton;
        private Framework.Controls.TrafodionLabel _theNoteLabel;
    }
}