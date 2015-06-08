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
﻿namespace Trafodion.Manager.MetricMiner.Controls
{
    partial class DrillDownMappingUserControl
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
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAssociationReason = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theMappingDetailsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theDetailedBrowser = new System.Windows.Forms.WebBrowser();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theWidgetMappingPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theEditButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theRemoveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionGroupBox2.SuspendLayout();
            this._theMappingDetailsGroupBox.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Controls.Add(this._theAssociationReason);
            this.TrafodionGroupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(0, 119);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(526, 107);
            this.TrafodionGroupBox2.TabIndex = 2;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "Link Reason (Describes the association between the reports)";
            // 
            // _theAssociationReason
            // 
            this._theAssociationReason.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAssociationReason.Enabled = false;
            this._theAssociationReason.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAssociationReason.Location = new System.Drawing.Point(3, 17);
            this._theAssociationReason.Multiline = true;
            this._theAssociationReason.Name = "_theAssociationReason";
            this._theAssociationReason.Size = new System.Drawing.Size(520, 87);
            this._theAssociationReason.TabIndex = 0;
            // 
            // _theMappingDetailsGroupBox
            // 
            this._theMappingDetailsGroupBox.Controls.Add(this._theDetailedBrowser);
            this._theMappingDetailsGroupBox.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theMappingDetailsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theMappingDetailsGroupBox.Location = new System.Drawing.Point(0, 226);
            this._theMappingDetailsGroupBox.Name = "_theMappingDetailsGroupBox";
            this._theMappingDetailsGroupBox.Size = new System.Drawing.Size(526, 145);
            this._theMappingDetailsGroupBox.TabIndex = 1;
            this._theMappingDetailsGroupBox.TabStop = false;
            this._theMappingDetailsGroupBox.Text = "Linked Columns";
            // 
            // _theDetailedBrowser
            // 
            this._theDetailedBrowser.AllowNavigation = false;
            this._theDetailedBrowser.AllowWebBrowserDrop = false;
            this._theDetailedBrowser.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theDetailedBrowser.IsWebBrowserContextMenuEnabled = false;
            this._theDetailedBrowser.Location = new System.Drawing.Point(3, 17);
            this._theDetailedBrowser.MinimumSize = new System.Drawing.Size(20, 20);
            this._theDetailedBrowser.Name = "_theDetailedBrowser";
            this._theDetailedBrowser.Size = new System.Drawing.Size(520, 125);
            this._theDetailedBrowser.TabIndex = 0;
            this._theDetailedBrowser.WebBrowserShortcutsEnabled = false;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._theWidgetMappingPanel);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionPanel1);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(526, 119);
            this.TrafodionGroupBox1.TabIndex = 0;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Linked Reports";
            // 
            // _theWidgetMappingPanel
            // 
            this._theWidgetMappingPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetMappingPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetMappingPanel.Location = new System.Drawing.Point(3, 17);
            this._theWidgetMappingPanel.Name = "_theWidgetMappingPanel";
            this._theWidgetMappingPanel.Size = new System.Drawing.Size(520, 67);
            this._theWidgetMappingPanel.TabIndex = 3;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._theEditButton);
            this.TrafodionPanel1.Controls.Add(this._theAddButton);
            this.TrafodionPanel1.Controls.Add(this._theRemoveButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(3, 84);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(520, 32);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // _theEditButton
            // 
            this._theEditButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theEditButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theEditButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theEditButton.Location = new System.Drawing.Point(363, 3);
            this._theEditButton.Name = "_theEditButton";
            this._theEditButton.Size = new System.Drawing.Size(75, 23);
            this._theEditButton.TabIndex = 2;
            this._theEditButton.Text = "Edit";
            this._theEditButton.UseVisualStyleBackColor = true;
            this._theEditButton.Click += new System.EventHandler(this._theEditButton_Click);
            // 
            // _theAddButton
            // 
            this._theAddButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theAddButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAddButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAddButton.Location = new System.Drawing.Point(282, 3);
            this._theAddButton.Name = "_theAddButton";
            this._theAddButton.Size = new System.Drawing.Size(75, 23);
            this._theAddButton.TabIndex = 1;
            this._theAddButton.Text = "Add";
            this._theAddButton.UseVisualStyleBackColor = true;
            this._theAddButton.Click += new System.EventHandler(this._theAddButton_Click);
            // 
            // _theRemoveButton
            // 
            this._theRemoveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theRemoveButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theRemoveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRemoveButton.Location = new System.Drawing.Point(442, 3);
            this._theRemoveButton.Name = "_theRemoveButton";
            this._theRemoveButton.Size = new System.Drawing.Size(75, 23);
            this._theRemoveButton.TabIndex = 0;
            this._theRemoveButton.Text = "Remove";
            this._theRemoveButton.UseVisualStyleBackColor = true;
            this._theRemoveButton.Click += new System.EventHandler(this._theRemoveButton_Click);
            // 
            // DrillDownMappingUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionGroupBox2);
            this.Controls.Add(this._theMappingDetailsGroupBox);
            this.Controls.Add(this.TrafodionGroupBox1);
            this.Name = "DrillDownMappingUserControl";
            this.Size = new System.Drawing.Size(526, 371);
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox2.PerformLayout();
            this._theMappingDetailsGroupBox.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theAddButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theRemoveButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theMappingDetailsGroupBox;
        private System.Windows.Forms.WebBrowser _theDetailedBrowser;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theEditButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theAssociationReason;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetMappingPanel;

    }
}
