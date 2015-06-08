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
﻿namespace Trafodion.Manager.Framework.Controls
{
    partial class DataGridViewButtonsUserControl
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
            this._clipboardButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._explorerButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._spreadsheetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._fileButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.SuspendLayout();
            // 
            // _clipboardButton
            // 
            this._clipboardButton.AutoSize = true;
            this._clipboardButton.Dock = System.Windows.Forms.DockStyle.Right;
            this._clipboardButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._clipboardButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._clipboardButton.Location = new System.Drawing.Point(376, 4);
            this._clipboardButton.Name = "_clipboardButton";
            this._clipboardButton.Size = new System.Drawing.Size(105, 23);
            this._clipboardButton.TabIndex = 3;
            this._clipboardButton.Text = "Data to &Clipboard";
            this._clipboardButton.UseVisualStyleBackColor = true;
            this._clipboardButton.Click += new System.EventHandler(this.theClipboadButtonClick);
            // 
            // _explorerButton
            // 
            this._explorerButton.AutoSize = true;
            this._explorerButton.Dock = System.Windows.Forms.DockStyle.Right;
            this._explorerButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._explorerButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._explorerButton.Location = new System.Drawing.Point(481, 4);
            this._explorerButton.Name = "_explorerButton";
            this._explorerButton.Size = new System.Drawing.Size(115, 23);
            this._explorerButton.TabIndex = 4;
            this._explorerButton.Text = "Data to E&xplorer ...";
            this._explorerButton.UseVisualStyleBackColor = true;
            this._explorerButton.Click += new System.EventHandler(this.theBrowserButtonClick);
            // 
            // _spreadsheetButton
            // 
            this._spreadsheetButton.AutoSize = true;
            this._spreadsheetButton.Dock = System.Windows.Forms.DockStyle.Right;
            this._spreadsheetButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._spreadsheetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._spreadsheetButton.Location = new System.Drawing.Point(596, 4);
            this._spreadsheetButton.Name = "_spreadsheetButton";
            this._spreadsheetButton.Size = new System.Drawing.Size(139, 23);
            this._spreadsheetButton.TabIndex = 2;
            this._spreadsheetButton.Text = "Data to &Spreadsheet ... ";
            this._spreadsheetButton.UseVisualStyleBackColor = true;
            this._spreadsheetButton.Click += new System.EventHandler(this.theSpreadsheetButtonClick);
            // 
            // _fileButton
            // 
            this._fileButton.AutoSize = true;
            this._fileButton.Dock = System.Windows.Forms.DockStyle.Right;
            this._fileButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._fileButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._fileButton.Location = new System.Drawing.Point(735, 4);
            this._fileButton.Name = "_fileButton";
            this._fileButton.Size = new System.Drawing.Size(91, 23);
            this._fileButton.TabIndex = 1;
            this._fileButton.Text = "Data to F&ile ...";
            this._fileButton.UseVisualStyleBackColor = true;
            this._fileButton.Click += new System.EventHandler(this.theExportButtonClick);
            // 
            // DataGridViewButtonsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._clipboardButton);
            this.Controls.Add(this._explorerButton);
            this.Controls.Add(this._spreadsheetButton);
            this.Controls.Add(this._fileButton);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "DataGridViewButtonsUserControl";
            this.Padding = new System.Windows.Forms.Padding(4);
            this.Size = new System.Drawing.Size(830, 31);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionButton _fileButton;
        private TrafodionButton _spreadsheetButton;
        private TrafodionButton _explorerButton;
        private TrafodionButton _clipboardButton;
    }
}
