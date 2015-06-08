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
    partial class ComponentPrivilegesUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ComponentPrivilegesUserControl));
            this._componentPrivilegesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._comPrivilegesGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.usersToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.addComPrivToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.deleteComPrivToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._theRemoveAllComPrivStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this._componentPrivilegesGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._comPrivilegesGrid)).BeginInit();
            this.usersToolStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // _componentPrivilegesGroupBox
            // 
            this._componentPrivilegesGroupBox.Controls.Add(this._comPrivilegesGrid);
            this._componentPrivilegesGroupBox.Controls.Add(this.usersToolStrip);
            this._componentPrivilegesGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._componentPrivilegesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._componentPrivilegesGroupBox.Location = new System.Drawing.Point(0, 0);
            this._componentPrivilegesGroupBox.Name = "_componentPrivilegesGroupBox";
            this._componentPrivilegesGroupBox.Size = new System.Drawing.Size(401, 306);
            this._componentPrivilegesGroupBox.TabIndex = 6;
            this._componentPrivilegesGroupBox.TabStop = false;
            this._componentPrivilegesGroupBox.Text = "Component Privileges";
            // 
            // _comPrivilegesGrid
            // 
            this._comPrivilegesGrid.AllowColumnFilter = true;
            this._comPrivilegesGrid.AllowWordWrap = false;
            this._comPrivilegesGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_comPrivilegesGrid.AlwaysHiddenColumnNames")));
            this._comPrivilegesGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._comPrivilegesGrid.CurrentFilter = null;
            this._comPrivilegesGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._comPrivilegesGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._comPrivilegesGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._comPrivilegesGrid.Header.Height = 20;
            this._comPrivilegesGrid.HelpTopic = "";
            this._comPrivilegesGrid.Location = new System.Drawing.Point(3, 42);
            this._comPrivilegesGrid.Name = "_comPrivilegesGrid";
            this._comPrivilegesGrid.ReadOnly = true;
            this._comPrivilegesGrid.RowMode = true;
            this._comPrivilegesGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._comPrivilegesGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._comPrivilegesGrid.SearchAsType.SearchCol = null;
            this._comPrivilegesGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            this._comPrivilegesGrid.Size = new System.Drawing.Size(395, 261);
            this._comPrivilegesGrid.TabIndex = 1;
            this._comPrivilegesGrid.TreeCol = null;
            this._comPrivilegesGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._comPrivilegesGrid.WordWrap = false;
            // 
            // usersToolStrip
            // 
            this.usersToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addComPrivToolStripButton,
            this.deleteComPrivToolStripButton,
            this.toolStripSeparator1,
            this._theRemoveAllComPrivStripButton,
            this.toolStripSeparator4});
            this.usersToolStrip.Location = new System.Drawing.Point(3, 17);
            this.usersToolStrip.Name = "usersToolStrip";
            this.usersToolStrip.Size = new System.Drawing.Size(395, 25);
            this.usersToolStrip.TabIndex = 0;
            this.usersToolStrip.Text = "TrafodionToolStrip2";
            // 
            // addComPrivToolStripButton
            // 
            this.addComPrivToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.addComPrivToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("addComPrivToolStripButton.Image")));
            this.addComPrivToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.addComPrivToolStripButton.Name = "addComPrivToolStripButton";
            this.addComPrivToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.addComPrivToolStripButton.Text = "Add Component Privileges...";
            this.addComPrivToolStripButton.Click += new System.EventHandler(this.addComPrivToolStripButton_Click);
            // 
            // deleteComPrivToolStripButton
            // 
            this.deleteComPrivToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.deleteComPrivToolStripButton.Enabled = false;
            this.deleteComPrivToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("deleteComPrivToolStripButton.Image")));
            this.deleteComPrivToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.deleteComPrivToolStripButton.Name = "deleteComPrivToolStripButton";
            this.deleteComPrivToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.deleteComPrivToolStripButton.Text = "Remove selected component privilege(s)";
            this.deleteComPrivToolStripButton.Click += new System.EventHandler(this.deleteComPrivToolStripButton_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _theRemoveAllComPrivStripButton
            // 
            this._theRemoveAllComPrivStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRemoveAllComPrivStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRemoveAllComPrivStripButton.Image")));
            this._theRemoveAllComPrivStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRemoveAllComPrivStripButton.Name = "_theRemoveAllComPrivStripButton";
            this._theRemoveAllComPrivStripButton.Size = new System.Drawing.Size(23, 22);
            this._theRemoveAllComPrivStripButton.Text = "Remove All component privileges";
            this._theRemoveAllComPrivStripButton.ToolTipText = "Remove all component privileges";
            this._theRemoveAllComPrivStripButton.Click += new System.EventHandler(this._theRemoveAllComPrivStripButton_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(6, 25);
            // 
            // ComponentPrivilegesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._componentPrivilegesGroupBox);
            this.Name = "ComponentPrivilegesUserControl";
            this.Size = new System.Drawing.Size(401, 306);
            this._componentPrivilegesGroupBox.ResumeLayout(false);
            this._componentPrivilegesGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._comPrivilegesGrid)).EndInit();
            this.usersToolStrip.ResumeLayout(false);
            this.usersToolStrip.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _componentPrivilegesGroupBox;
        private Framework.Controls.TrafodionIGrid _comPrivilegesGrid;
        private Framework.Controls.TrafodionToolStrip usersToolStrip;
        private System.Windows.Forms.ToolStripButton addComPrivToolStripButton;
        private System.Windows.Forms.ToolStripButton deleteComPrivToolStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton _theRemoveAllComPrivStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
    }
}
