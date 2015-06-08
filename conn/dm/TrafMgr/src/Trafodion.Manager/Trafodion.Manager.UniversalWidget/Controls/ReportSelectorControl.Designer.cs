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
﻿namespace Trafodion.Manager.UniversalWidget.Controls
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
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theWidgetMappingListBox = new Trafodion.Manager.Framework.Controls.TrafodionListBox();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDeleteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theEditButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theMappingDetailsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theParameterInputPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox1.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            this._theMappingDetailsGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this._theWidgetMappingListBox);
            this.oneGuiGroupBox1.Controls.Add(this.oneGuiPanel1);
            this.oneGuiGroupBox1.Dock = System.Windows.Forms.DockStyle.Top;
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(565, 119);
            this.oneGuiGroupBox1.TabIndex = 1;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Linked Reports";
            // 
            // _theWidgetMappingListBox
            // 
            this._theWidgetMappingListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetMappingListBox.Font = new System.Drawing.Font("Arial", 8F);
            this._theWidgetMappingListBox.FormattingEnabled = true;
            this._theWidgetMappingListBox.ItemHeight = 14;
            this._theWidgetMappingListBox.Location = new System.Drawing.Point(3, 16);
            this._theWidgetMappingListBox.Name = "_theWidgetMappingListBox";
            this._theWidgetMappingListBox.Size = new System.Drawing.Size(471, 88);
            this._theWidgetMappingListBox.TabIndex = 2;
            this._theWidgetMappingListBox.SelectedIndexChanged += new System.EventHandler(this._theWidgetMappingListBox_SelectedIndexChanged);
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.Controls.Add(this._theDeleteButton);
            this.oneGuiPanel1.Controls.Add(this._theEditButton);
            this.oneGuiPanel1.Controls.Add(this._theAddButton);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Right;
            this.oneGuiPanel1.Location = new System.Drawing.Point(474, 16);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(88, 100);
            this.oneGuiPanel1.TabIndex = 3;
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
            this._theMappingDetailsGroupBox.Location = new System.Drawing.Point(0, 119);
            this._theMappingDetailsGroupBox.Name = "_theMappingDetailsGroupBox";
            this._theMappingDetailsGroupBox.Size = new System.Drawing.Size(565, 148);
            this._theMappingDetailsGroupBox.TabIndex = 2;
            this._theMappingDetailsGroupBox.TabStop = false;
            this._theMappingDetailsGroupBox.Text = "Passed Values";
            // 
            // _theParameterInputPanel
            // 
            this._theParameterInputPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theParameterInputPanel.Location = new System.Drawing.Point(3, 16);
            this._theParameterInputPanel.Name = "_theParameterInputPanel";
            this._theParameterInputPanel.Size = new System.Drawing.Size(559, 129);
            this._theParameterInputPanel.TabIndex = 0;
            // 
            // ReportSelectorControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theMappingDetailsGroupBox);
            this.Controls.Add(this.oneGuiGroupBox1);
            this.Name = "ReportSelectorControl";
            this.Size = new System.Drawing.Size(565, 267);
            this.oneGuiGroupBox1.ResumeLayout(false);
            this.oneGuiPanel1.ResumeLayout(false);
            this._theMappingDetailsGroupBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionListBox _theWidgetMappingListBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theMappingDetailsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theParameterInputPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theEditButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theAddButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theDeleteButton;
    }
}
