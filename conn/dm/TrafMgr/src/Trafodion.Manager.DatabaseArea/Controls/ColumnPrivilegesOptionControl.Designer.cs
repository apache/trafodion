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
    partial class ColumnPrivilegesOptionControl
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
            this._colPrivGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._colPrivListBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckedListBox();
            this._selColRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._allColRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._noColRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._colPrivGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _colPrivGroupBox
            // 
            this._colPrivGroupBox.Controls.Add(this._colPrivListBox);
            this._colPrivGroupBox.Controls.Add(this._selColRadioButton);
            this._colPrivGroupBox.Controls.Add(this._allColRadioButton);
            this._colPrivGroupBox.Controls.Add(this._noColRadioButton);
            this._colPrivGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._colPrivGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._colPrivGroupBox.Location = new System.Drawing.Point(0, 0);
            this._colPrivGroupBox.Name = "_colPrivGroupBox";
            this._colPrivGroupBox.Size = new System.Drawing.Size(235, 468);
            this._colPrivGroupBox.TabIndex = 7;
            this._colPrivGroupBox.TabStop = false;
            this._colPrivGroupBox.Text = "Privileges";
            // 
            // _colPrivListBox
            // 
            this._colPrivListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._colPrivListBox.CheckOnClick = true;
            this._colPrivListBox.FormattingEnabled = true;
            this._colPrivListBox.HorizontalScrollbar = true;
            this._colPrivListBox.Location = new System.Drawing.Point(8, 106);
            this._colPrivListBox.Name = "_colPrivListBox";
            this._colPrivListBox.Size = new System.Drawing.Size(220, 356);
            this._colPrivListBox.TabIndex = 7;
            this._colPrivListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this._colPrivListBox_ItemCheck);
            this._colPrivListBox.MouseMove += new System.Windows.Forms.MouseEventHandler(this.onMouseMove);
            // 
            // _selColRadioButton
            // 
            this._selColRadioButton.AutoSize = true;
            this._selColRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._selColRadioButton.Location = new System.Drawing.Point(12, 78);
            this._selColRadioButton.Name = "_selColRadioButton";
            this._selColRadioButton.Size = new System.Drawing.Size(103, 18);
            this._selColRadioButton.TabIndex = 7;
            this._selColRadioButton.Text = "These Columns";
            this._selColRadioButton.UseVisualStyleBackColor = true;
            this._selColRadioButton.CheckedChanged += new System.EventHandler(this._selColRadioButton_CheckedChanged);
            // 
            // _allColRadioButton
            // 
            this._allColRadioButton.AutoSize = true;
            this._allColRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allColRadioButton.Location = new System.Drawing.Point(12, 54);
            this._allColRadioButton.Name = "_allColRadioButton";
            this._allColRadioButton.Size = new System.Drawing.Size(85, 18);
            this._allColRadioButton.TabIndex = 7;
            this._allColRadioButton.TabStop = true;
            this._allColRadioButton.Text = "All Columns";
            this._allColRadioButton.UseVisualStyleBackColor = true;
            this._allColRadioButton.Click += new System.EventHandler(this._allColRadioButton_Click);
            // 
            // _noColRadioButton
            // 
            this._noColRadioButton.AutoSize = true;
            this._noColRadioButton.Checked = true;
            this._noColRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._noColRadioButton.Location = new System.Drawing.Point(12, 30);
            this._noColRadioButton.Name = "_noColRadioButton";
            this._noColRadioButton.Size = new System.Drawing.Size(87, 18);
            this._noColRadioButton.TabIndex = 7;
            this._noColRadioButton.TabStop = true;
            this._noColRadioButton.Text = "No Columns";
            this._noColRadioButton.UseVisualStyleBackColor = true;
            this._noColRadioButton.Click += new System.EventHandler(this._noColRadioButton_Click);
            // 
            // ColumnPrivilegesOptionControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._colPrivGroupBox);
            this.Name = "ColumnPrivilegesOptionControl";
            this.Size = new System.Drawing.Size(235, 468);
            this._colPrivGroupBox.ResumeLayout(false);
            this._colPrivGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _colPrivGroupBox;
        private Framework.Controls.TrafodionCheckedListBox _colPrivListBox;
        private Framework.Controls.TrafodionRadioButton _selColRadioButton;
        private Framework.Controls.TrafodionRadioButton _allColRadioButton;
        private Framework.Controls.TrafodionRadioButton _noColRadioButton;
        private Framework.Controls.TrafodionToolTip _toolTip;
    }
}
