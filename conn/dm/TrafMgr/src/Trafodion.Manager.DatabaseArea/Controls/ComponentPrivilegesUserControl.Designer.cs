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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ComponentPrivilegesUserControl));
            this._privilegesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._privilegesGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this._privilegesToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._addPrivilegeToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._deletePrivilegeToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._componentHeaderPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._componentComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._componentNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._grantedByPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._grantedByComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._grantedByLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._privilegesGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._privilegesGrid)).BeginInit();
            this._privilegesToolStrip.SuspendLayout();
            this._componentHeaderPanel.SuspendLayout();
            this._grantedByPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _privilegesGroupBox
            // 
            this._privilegesGroupBox.Controls.Add(this._privilegesGrid);
            this._privilegesGroupBox.Controls.Add(this._privilegesToolStrip);
            this._privilegesGroupBox.Controls.Add(this._componentHeaderPanel);
            this._privilegesGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._privilegesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._privilegesGroupBox.Location = new System.Drawing.Point(0, 0);
            this._privilegesGroupBox.Name = "_privilegesGroupBox";
            this._privilegesGroupBox.Size = new System.Drawing.Size(722, 408);
            this._privilegesGroupBox.TabIndex = 1;
            this._privilegesGroupBox.TabStop = false;
            this._privilegesGroupBox.Text = "Privileges";
            // 
            // _privilegesGrid
            // 
            this._privilegesGrid.AllowColumnFilter = true;
            this._privilegesGrid.AllowWordWrap = false;
            this._privilegesGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_privilegesGrid.AlwaysHiddenColumnNames")));
            this._privilegesGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._privilegesGrid.CurrentFilter = null;
            this._privilegesGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._privilegesGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._privilegesGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._privilegesGrid.Header.Height = 20;
            this._privilegesGrid.HelpTopic = "";
            this._privilegesGrid.Location = new System.Drawing.Point(3, 56);
            this._privilegesGrid.Name = "_privilegesGrid";
            this._privilegesGrid.ReadOnly = true;
            this._privilegesGrid.RowMode = true;
            this._privilegesGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._privilegesGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._privilegesGrid.SearchAsType.SearchCol = null;
            this._privilegesGrid.Size = new System.Drawing.Size(716, 349);
            this._privilegesGrid.TabIndex = 0;
            this._privilegesGrid.TreeCol = null;
            this._privilegesGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._privilegesGrid.WordWrap = false;
            // 
            // _privilegesToolStrip
            // 
            this._privilegesToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._addPrivilegeToolStripButton,
            this._deletePrivilegeToolStripButton});
            this._privilegesToolStrip.Location = new System.Drawing.Point(3, 56);
            this._privilegesToolStrip.Name = "_privilegesToolStrip";
            this._privilegesToolStrip.Size = new System.Drawing.Size(716, 25);
            this._privilegesToolStrip.TabIndex = 2;
            this._privilegesToolStrip.Text = "TrafodionToolStrip1";
            this._privilegesToolStrip.Visible = false;
            // 
            // _addPrivilegeToolStripButton
            // 
            this._addPrivilegeToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._addPrivilegeToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_addPrivilegeToolStripButton.Image")));
            this._addPrivilegeToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._addPrivilegeToolStripButton.Name = "_addPrivilegeToolStripButton";
            this._addPrivilegeToolStripButton.Size = new System.Drawing.Size(23, 22);
            this._addPrivilegeToolStripButton.Text = "toolStripButton3";
            this._addPrivilegeToolStripButton.ToolTipText = "Add additional privileges to grantee";
            // 
            // _deletePrivilegeToolStripButton
            // 
            this._deletePrivilegeToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._deletePrivilegeToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_deletePrivilegeToolStripButton.Image")));
            this._deletePrivilegeToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._deletePrivilegeToolStripButton.Name = "_deletePrivilegeToolStripButton";
            this._deletePrivilegeToolStripButton.Size = new System.Drawing.Size(23, 22);
            this._deletePrivilegeToolStripButton.Text = "toolStripButton4";
            this._deletePrivilegeToolStripButton.ToolTipText = "Remove selected privileges from grantee";
            // 
            // _componentHeaderPanel
            // 
            this._componentHeaderPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._componentHeaderPanel.Controls.Add(this._componentComboBox);
            this._componentHeaderPanel.Controls.Add(this._componentNameLabel);
            this._componentHeaderPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._componentHeaderPanel.Location = new System.Drawing.Point(3, 17);
            this._componentHeaderPanel.Name = "_componentHeaderPanel";
            this._componentHeaderPanel.Size = new System.Drawing.Size(716, 39);
            this._componentHeaderPanel.TabIndex = 2;
            this._componentHeaderPanel.Visible = false;
            // 
            // _componentComboBox
            // 
            this._componentComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._componentComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._componentComboBox.FormattingEnabled = true;
            this._componentComboBox.Location = new System.Drawing.Point(86, 8);
            this._componentComboBox.Name = "_componentComboBox";
            this._componentComboBox.Size = new System.Drawing.Size(145, 21);
            this._componentComboBox.TabIndex = 0;
            // 
            // _componentNameLabel
            // 
            this._componentNameLabel.AutoSize = true;
            this._componentNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._componentNameLabel.Location = new System.Drawing.Point(9, 11);
            this._componentNameLabel.Name = "_componentNameLabel";
            this._componentNameLabel.Size = new System.Drawing.Size(72, 13);
            this._componentNameLabel.TabIndex = 1;
            this._componentNameLabel.Text = "Component";
            // 
            // _grantedByPanel
            // 
            this._grantedByPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._grantedByPanel.Controls.Add(this._grantedByComboBox);
            this._grantedByPanel.Controls.Add(this._grantedByLabel);
            this._grantedByPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._grantedByPanel.Location = new System.Drawing.Point(0, 408);
            this._grantedByPanel.Name = "_grantedByPanel";
            this._grantedByPanel.Size = new System.Drawing.Size(722, 50);
            this._grantedByPanel.TabIndex = 3;
            // 
            // _grantedByComboBox
            // 
            this._grantedByComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._grantedByComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._grantedByComboBox.FormattingEnabled = true;
            this._grantedByComboBox.Location = new System.Drawing.Point(79, 15);
            this._grantedByComboBox.Name = "_grantedByComboBox";
            this._grantedByComboBox.Size = new System.Drawing.Size(630, 21);
            this._grantedByComboBox.TabIndex = 8;
            // 
            // _grantedByLabel
            // 
            this._grantedByLabel.AutoSize = true;
            this._grantedByLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._grantedByLabel.Location = new System.Drawing.Point(3, 18);
            this._grantedByLabel.Name = "_grantedByLabel";
            this._grantedByLabel.Size = new System.Drawing.Size(70, 13);
            this._grantedByLabel.TabIndex = 7;
            this._grantedByLabel.Text = "Granted By";
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // ComponentPrivilegesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._privilegesGroupBox);
            this.Controls.Add(this._grantedByPanel);
            this.Name = "ComponentPrivilegesUserControl";
            this.Size = new System.Drawing.Size(722, 458);
            this._privilegesGroupBox.ResumeLayout(false);
            this._privilegesGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._privilegesGrid)).EndInit();
            this._privilegesToolStrip.ResumeLayout(false);
            this._privilegesToolStrip.PerformLayout();
            this._componentHeaderPanel.ResumeLayout(false);
            this._componentHeaderPanel.PerformLayout();
            this._grantedByPanel.ResumeLayout(false);
            this._grantedByPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _privilegesGroupBox;
        private Framework.Controls.TrafodionIGrid _privilegesGrid;
        private Framework.Controls.TrafodionToolStrip _privilegesToolStrip;
        private System.Windows.Forms.ToolStripButton _addPrivilegeToolStripButton;
        private System.Windows.Forms.ToolStripButton _deletePrivilegeToolStripButton;
        private Framework.Controls.TrafodionPanel _componentHeaderPanel;
        private Framework.Controls.TrafodionComboBox _componentComboBox;
        private Framework.Controls.TrafodionLabel _componentNameLabel;
        private Framework.Controls.TrafodionPanel _grantedByPanel;
        private Framework.Controls.TrafodionLabel _grantedByLabel;
        private Framework.Controls.TrafodionComboBox _grantedByComboBox;
        private Framework.Controls.TrafodionToolTip _toolTip;

    }
}
