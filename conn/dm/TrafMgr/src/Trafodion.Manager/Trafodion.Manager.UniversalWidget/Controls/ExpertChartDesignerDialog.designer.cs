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
    partial class ExpertChartDesignerDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExpertChartDesignerDialog));
            this._theFlowLayoutPanel = new System.Windows.Forms.FlowLayoutPanel();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theApplyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOkButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._upperPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFlowLayoutPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theFlowLayoutPanel
            // 
            this._theFlowLayoutPanel.Controls.Add(this._theCancelButton);
            this._theFlowLayoutPanel.Controls.Add(this._theApplyButton);
            this._theFlowLayoutPanel.Controls.Add(this._theOkButton);
            this._theFlowLayoutPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theFlowLayoutPanel.Location = new System.Drawing.Point(0, 310);
            this._theFlowLayoutPanel.Name = "_theFlowLayoutPanel";
            this._theFlowLayoutPanel.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this._theFlowLayoutPanel.Size = new System.Drawing.Size(830, 31);
            this._theFlowLayoutPanel.TabIndex = 2;
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(752, 3);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 0;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _theApplyButton
            // 
            this._theApplyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theApplyButton.Location = new System.Drawing.Point(671, 3);
            this._theApplyButton.Name = "_theApplyButton";
            this._theApplyButton.Size = new System.Drawing.Size(75, 23);
            this._theApplyButton.TabIndex = 1;
            this._theApplyButton.Text = "&Apply";
            this._theApplyButton.UseVisualStyleBackColor = true;
            this._theApplyButton.Click += new System.EventHandler(this._theApplyButton_Click);
            // 
            // _theOkButton
            // 
            this._theOkButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOkButton.Location = new System.Drawing.Point(590, 3);
            this._theOkButton.Name = "_theOkButton";
            this._theOkButton.Size = new System.Drawing.Size(75, 23);
            this._theOkButton.TabIndex = 2;
            this._theOkButton.Text = "&OK";
            this._theOkButton.UseVisualStyleBackColor = true;
            this._theOkButton.Click += new System.EventHandler(this._theOkButton_Click);
            // 
            // _upperPanel
            // 
            this._upperPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._upperPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._upperPanel.Location = new System.Drawing.Point(0, 0);
            this._upperPanel.Name = "_upperPanel";
            this._upperPanel.Size = new System.Drawing.Size(830, 310);
            this._upperPanel.TabIndex = 3;
            // 
            // ExpertChartDesignerDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(830, 341);
            this.Controls.Add(this._upperPanel);
            this.Controls.Add(this._theFlowLayoutPanel);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ExpertChartDesignerDialog";
            this.Text = "Expert Chart Designer";
            this._theFlowLayoutPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel _theFlowLayoutPanel;
        private Framework.Controls.TrafodionButton _theCancelButton;
        private Framework.Controls.TrafodionButton _theApplyButton;
        private Framework.Controls.TrafodionButton _theOkButton;
        private Framework.Controls.TrafodionPanel _upperPanel;
    }
}