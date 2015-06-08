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
    partial class ReportSelectorControl
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
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theWidgetMappingGrid = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDeleteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theEditButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theMappingDetailsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theParameterInputPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAssociationReason = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this._theMappingDetailsGroupBox.SuspendLayout();
            this.TrafodionGroupBox2.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._theWidgetMappingGrid);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionPanel1);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(551, 119);
            this.TrafodionGroupBox1.TabIndex = 1;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Linked Reports";
            // 
            // _theWidgetMappingGrid
            // 
            this._theWidgetMappingGrid.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theWidgetMappingGrid.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetMappingGrid.Location = new System.Drawing.Point(6, 20);
            this._theWidgetMappingGrid.Name = "_theWidgetMappingGrid";
            this._theWidgetMappingGrid.Size = new System.Drawing.Size(448, 93);
            this._theWidgetMappingGrid.TabIndex = 4;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._theDeleteButton);
            this.TrafodionPanel1.Controls.Add(this._theEditButton);
            this.TrafodionPanel1.Controls.Add(this._theAddButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Right;
            this.TrafodionPanel1.Location = new System.Drawing.Point(460, 17);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(88, 99);
            this.TrafodionPanel1.TabIndex = 3;
            // 
            // _theDeleteButton
            // 
            this._theDeleteButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theDeleteButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDeleteButton.Location = new System.Drawing.Point(6, 61);
            this._theDeleteButton.Name = "_theDeleteButton";
            this._theDeleteButton.Size = new System.Drawing.Size(75, 23);
            this._theDeleteButton.TabIndex = 2;
            this._theDeleteButton.Text = "Delete";
            this._theDeleteButton.UseVisualStyleBackColor = true;
            this._theDeleteButton.Click += new System.EventHandler(this._theDeleteButton_Click);
            // 
            // _theEditButton
            // 
            this._theEditButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theEditButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theEditButton.Location = new System.Drawing.Point(6, 32);
            this._theEditButton.Name = "_theEditButton";
            this._theEditButton.Size = new System.Drawing.Size(75, 23);
            this._theEditButton.TabIndex = 1;
            this._theEditButton.Text = "Edit";
            this._theEditButton.UseVisualStyleBackColor = true;
            this._theEditButton.Click += new System.EventHandler(this._theEditButton_Click);
            // 
            // _theAddButton
            // 
            this._theAddButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAddButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAddButton.Location = new System.Drawing.Point(6, 3);
            this._theAddButton.Name = "_theAddButton";
            this._theAddButton.Size = new System.Drawing.Size(75, 23);
            this._theAddButton.TabIndex = 0;
            this._theAddButton.Text = "Add";
            this._theAddButton.UseVisualStyleBackColor = true;
            this._theAddButton.Click += new System.EventHandler(this._theAddButton_Click);
            // 
            // _theMappingDetailsGroupBox
            // 
            this._theMappingDetailsGroupBox.Controls.Add(this._theParameterInputPanel);
            this._theMappingDetailsGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMappingDetailsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theMappingDetailsGroupBox.Location = new System.Drawing.Point(0, 206);
            this._theMappingDetailsGroupBox.Name = "_theMappingDetailsGroupBox";
            this._theMappingDetailsGroupBox.Size = new System.Drawing.Size(551, 155);
            this._theMappingDetailsGroupBox.TabIndex = 2;
            this._theMappingDetailsGroupBox.TabStop = false;
            this._theMappingDetailsGroupBox.Text = "Passed Values";
            // 
            // _theParameterInputPanel
            // 
            this._theParameterInputPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theParameterInputPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theParameterInputPanel.Location = new System.Drawing.Point(3, 17);
            this._theParameterInputPanel.Name = "_theParameterInputPanel";
            this._theParameterInputPanel.Size = new System.Drawing.Size(545, 135);
            this._theParameterInputPanel.TabIndex = 0;
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Controls.Add(this._theAssociationReason);
            this.TrafodionGroupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(0, 119);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(551, 87);
            this.TrafodionGroupBox2.TabIndex = 3;
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
            this._theAssociationReason.Size = new System.Drawing.Size(545, 67);
            this._theAssociationReason.TabIndex = 0;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this.TrafodionGroupBox2);
            this.TrafodionPanel2.Controls.Add(this.TrafodionGroupBox1);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(551, 206);
            this.TrafodionPanel2.TabIndex = 4;
            // 
            // ReportSelectorControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theMappingDetailsGroupBox);
            this.Controls.Add(this.TrafodionPanel2);
            this.Name = "ReportSelectorControl";
            this.Size = new System.Drawing.Size(551, 361);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this._theMappingDetailsGroupBox.ResumeLayout(false);
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox2.PerformLayout();
            this.TrafodionPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theMappingDetailsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theParameterInputPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theAddButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theDeleteButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theEditButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theAssociationReason;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetMappingGrid;
    }
}
