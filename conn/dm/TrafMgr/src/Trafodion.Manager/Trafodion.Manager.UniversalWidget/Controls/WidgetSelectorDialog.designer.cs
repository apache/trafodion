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
    partial class WidgetSelectorDialog
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
            this.label1 = new System.Windows.Forms.Label();
            this._theAssociatedWidgets = new System.Windows.Forms.ComboBox();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theOkButton = new System.Windows.Forms.Button();
            this._theCancelButton = new System.Windows.Forms.Button();
            this._theReportSelectorControl = new Trafodion.Manager.UniversalWidget.Controls.ReportSelectorControl();
            this.oneGuiPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(306, 157);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(70, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Report Name";
            // 
            // _theAssociatedWidgets
            // 
            this._theAssociatedWidgets.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theAssociatedWidgets.FormattingEnabled = true;
            this._theAssociatedWidgets.Location = new System.Drawing.Point(390, 154);
            this._theAssociatedWidgets.Name = "_theAssociatedWidgets";
            this._theAssociatedWidgets.Size = new System.Drawing.Size(73, 21);
            this._theAssociatedWidgets.TabIndex = 1;
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.Controls.Add(this._theOkButton);
            this.oneGuiPanel1.Controls.Add(this._theCancelButton);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 252);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(553, 33);
            this.oneGuiPanel1.TabIndex = 4;
            // 
            // _theOkButton
            // 
            this._theOkButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theOkButton.Location = new System.Drawing.Point(434, 6);
            this._theOkButton.Name = "_theOkButton";
            this._theOkButton.Size = new System.Drawing.Size(55, 22);
            this._theOkButton.TabIndex = 2;
            this._theOkButton.Text = "Ok";
            this._theOkButton.UseVisualStyleBackColor = true;
            this._theOkButton.Click += new System.EventHandler(this._theOkButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.Location = new System.Drawing.Point(495, 6);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(55, 22);
            this._theCancelButton.TabIndex = 3;
            this._theCancelButton.Text = "Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _theReportSelectorControl
            // 
            this._theReportSelectorControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theReportSelectorControl.Location = new System.Drawing.Point(0, 0);
            this._theReportSelectorControl.Name = "_theReportSelectorControl";
            this._theReportSelectorControl.Padding = new System.Windows.Forms.Padding(3);
            this._theReportSelectorControl.Size = new System.Drawing.Size(553, 252);
            this._theReportSelectorControl.TabIndex = 5;
            // 
            // WidgetSelectorDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(553, 285);
            this.Controls.Add(this._theReportSelectorControl);
            this.Controls.Add(this.oneGuiPanel1);
            this.Controls.Add(this._theAssociatedWidgets);
            this.Controls.Add(this.label1);
            this.Name = "WidgetSelectorDialog";
            this.Text = "Select Report To Launch";
            this.oneGuiPanel1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox _theAssociatedWidgets;
        private System.Windows.Forms.Button _theOkButton;
        private System.Windows.Forms.Button _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private ReportSelectorControl _theReportSelectorControl;
    }
}