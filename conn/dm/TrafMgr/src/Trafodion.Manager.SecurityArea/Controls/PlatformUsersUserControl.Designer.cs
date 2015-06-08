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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class PlatformUsersUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._addLikeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._deleteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._editButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._addButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theExportButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel.SuspendLayout();
            this._theAdditionalButtonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _bottomPanel
            // 
            this._bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomPanel.Controls.Add(this._theAdditionalButtonPanel);
            this._bottomPanel.Controls.Add(this._theExportButtonPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 371);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.Size = new System.Drawing.Size(814, 33);
            this._bottomPanel.TabIndex = 0;
            // 
            // _theAdditionalButtonPanel
            // 
            this._theAdditionalButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalButtonPanel.Controls.Add(this._addLikeButton);
            this._theAdditionalButtonPanel.Controls.Add(this._deleteButton);
            this._theAdditionalButtonPanel.Controls.Add(this._editButton);
            this._theAdditionalButtonPanel.Controls.Add(this._addButton);
            this._theAdditionalButtonPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalButtonPanel.Location = new System.Drawing.Point(0, 0);
            this._theAdditionalButtonPanel.Name = "_theAdditionalButtonPanel";
            this._theAdditionalButtonPanel.Size = new System.Drawing.Size(326, 33);
            this._theAdditionalButtonPanel.TabIndex = 1;
            // 
            // _addLikeButton
            // 
            this._addLikeButton.AutoSize = true;
            this._addLikeButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._addLikeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._addLikeButton.Location = new System.Drawing.Point(73, 3);
            this._addLikeButton.Name = "_addLikeButton";
            this._addLikeButton.Size = new System.Drawing.Size(76, 23);
            this._addLikeButton.TabIndex = 3;
            this._addLikeButton.Text = "Add &Like ...";
            this._addLikeButton.UseVisualStyleBackColor = true;
            this._addLikeButton.Click += new System.EventHandler(this._addLikeButton_Click);
            // 
            // _deleteButton
            // 
            this._deleteButton.AutoSize = true;
            this._deleteButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._deleteButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._deleteButton.Location = new System.Drawing.Point(219, 3);
            this._deleteButton.Name = "_deleteButton";
            this._deleteButton.Size = new System.Drawing.Size(70, 23);
            this._deleteButton.TabIndex = 2;
            this._deleteButton.Text = "&Delete";
            this._deleteButton.UseVisualStyleBackColor = true;
            this._deleteButton.Click += new System.EventHandler(this._deleteButton_Click);
            // 
            // _editButton
            // 
            this._editButton.AutoSize = true;
            this._editButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._editButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._editButton.Location = new System.Drawing.Point(149, 3);
            this._editButton.Name = "_editButton";
            this._editButton.Size = new System.Drawing.Size(70, 23);
            this._editButton.TabIndex = 1;
            this._editButton.Text = "&Edit ...";
            this._editButton.UseVisualStyleBackColor = true;
            this._editButton.Click += new System.EventHandler(this._editButton_Click);
            // 
            // _addButton
            // 
            this._addButton.AutoSize = true;
            this._addButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._addButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._addButton.Location = new System.Drawing.Point(3, 3);
            this._addButton.Name = "_addButton";
            this._addButton.Size = new System.Drawing.Size(70, 23);
            this._addButton.TabIndex = 0;
            this._addButton.Text = "&Add ...";
            this._addButton.UseVisualStyleBackColor = true;
            this._addButton.Click += new System.EventHandler(this._addButton_Click);
            // 
            // _theExportButtonPanel
            // 
            this._theExportButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theExportButtonPanel.Dock = System.Windows.Forms.DockStyle.Right;
            this._theExportButtonPanel.Location = new System.Drawing.Point(326, 0);
            this._theExportButtonPanel.Name = "_theExportButtonPanel";
            this._theExportButtonPanel.Size = new System.Drawing.Size(488, 33);
            this._theExportButtonPanel.TabIndex = 0;
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 0);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(814, 371);
            this._theWidgetPanel.TabIndex = 1;
            // 
            // PlatformUsersUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theWidgetPanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "PlatformUsersUserControl";
            this.Size = new System.Drawing.Size(814, 404);
            this.Load += new System.EventHandler(this.PlatformUsersUserControl_Load);
            this._bottomPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theExportButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _deleteButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _editButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _addButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _addLikeButton;




    }
}
