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
    partial class ChartDesignerDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ChartDesignerDialog));
            this._theFlowLayoutPanel = new System.Windows.Forms.FlowLayoutPanel();
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theSaveButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theApplyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theResetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theUpperPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFlowLayoutPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theFlowLayoutPanel
            // 
            this._theFlowLayoutPanel.Controls.Add(this._helpButton);
            this._theFlowLayoutPanel.Controls.Add(this._theSaveButton);
            this._theFlowLayoutPanel.Controls.Add(this._theCancelButton);
            this._theFlowLayoutPanel.Controls.Add(this._theApplyButton);
            this._theFlowLayoutPanel.Controls.Add(this._theResetButton);
            this._theFlowLayoutPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theFlowLayoutPanel.Location = new System.Drawing.Point(0, 411);
            this._theFlowLayoutPanel.Name = "_theFlowLayoutPanel";
            this._theFlowLayoutPanel.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this._theFlowLayoutPanel.Size = new System.Drawing.Size(717, 31);
            this._theFlowLayoutPanel.TabIndex = 2;
            // 
            // _helpButton
            // 
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.Location = new System.Drawing.Point(639, 3);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 3;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // _theSaveButton
            // 
            this._theSaveButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSaveButton.Location = new System.Drawing.Point(558, 3);
            this._theSaveButton.Name = "_theSaveButton";
            this._theSaveButton.Size = new System.Drawing.Size(75, 23);
            this._theSaveButton.TabIndex = 2;
            this._theSaveButton.Text = "&Save";
            this._theSaveButton.UseVisualStyleBackColor = true;
            this._theSaveButton.Click += new System.EventHandler(this._theSaveButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(477, 3);
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
            this._theApplyButton.Location = new System.Drawing.Point(396, 3);
            this._theApplyButton.Name = "_theApplyButton";
            this._theApplyButton.Size = new System.Drawing.Size(75, 23);
            this._theApplyButton.TabIndex = 1;
            this._theApplyButton.Text = "&Apply";
            this._theApplyButton.UseVisualStyleBackColor = true;
            this._theApplyButton.Click += new System.EventHandler(this._theApplyButton_Click);
            // 
            // _theResetButton
            // 
            this._theResetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theResetButton.Location = new System.Drawing.Point(315, 3);
            this._theResetButton.Name = "_theResetButton";
            this._theResetButton.Size = new System.Drawing.Size(75, 23);
            this._theResetButton.TabIndex = 3;
            this._theResetButton.Text = "&Reset";
            this._theResetButton.UseVisualStyleBackColor = true;
            this._theResetButton.Click += new System.EventHandler(this._theResetButton_Click);
            // 
            // _theUpperPanel
            // 
            this._theUpperPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theUpperPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._theUpperPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theUpperPanel.Location = new System.Drawing.Point(0, 0);
            this._theUpperPanel.Name = "_theUpperPanel";
            this._theUpperPanel.Size = new System.Drawing.Size(717, 411);
            this._theUpperPanel.TabIndex = 3;
            // 
            // ChartDesignerDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(717, 442);
            this.Controls.Add(this._theUpperPanel);
            this.Controls.Add(this._theFlowLayoutPanel);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ChartDesignerDialog";
            this.Text = "Trafodion Database Manager - Chart Designer";
            this._theFlowLayoutPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel _theFlowLayoutPanel;
        private Framework.Controls.TrafodionButton _theCancelButton;
        private Framework.Controls.TrafodionButton _theApplyButton;
        private Framework.Controls.TrafodionButton _theSaveButton;
        private Framework.Controls.TrafodionButton _theResetButton;
        private Framework.Controls.TrafodionPanel _theUpperPanel;
        private Framework.Controls.TrafodionButton _helpButton;
    }
}