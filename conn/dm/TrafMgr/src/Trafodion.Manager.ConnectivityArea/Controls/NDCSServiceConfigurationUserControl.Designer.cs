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
﻿namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class WMSConfigurationUserControl
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
            this._configurationTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._configurationPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.theTopPanelLowerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.theTopPanelLowerLabelRight = new System.Windows.Forms.Label();
            this.theParentButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.thePreviousButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theNextButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theTopPanelUpperLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._configurationPanel.SuspendLayout();
            this.theTopPanel.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _configurationTabControl
            // 
            this._configurationTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._configurationTabControl.Location = new System.Drawing.Point(0, 0);
            this._configurationTabControl.Name = "_configurationTabControl";
            this._configurationTabControl.Padding = new System.Drawing.Point(10, 5);
            this._configurationTabControl.SelectedIndex = 0;
            this._configurationTabControl.Size = new System.Drawing.Size(697, 385);
            this._configurationTabControl.TabIndex = 0;
            this._configurationTabControl.Visible = false;
            // 
            // _configurationPanel
            // 
            this._configurationPanel.Controls.Add(this._configurationTabControl);
            this._configurationPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._configurationPanel.Location = new System.Drawing.Point(0, 61);
            this._configurationPanel.Name = "_configurationPanel";
            this._configurationPanel.Size = new System.Drawing.Size(697, 385);
            this._configurationPanel.TabIndex = 1;
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.Controls.Add(this.flowLayoutPanel1);
            this.theTopPanel.Controls.Add(this.theParentButton);
            this.theTopPanel.Controls.Add(this.thePreviousButton);
            this.theTopPanel.Controls.Add(this.theNextButton);
            this.theTopPanel.Controls.Add(this.theTopPanelUpperLabel);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(697, 61);
            this.theTopPanel.TabIndex = 4;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.flowLayoutPanel1.Controls.Add(this.theTopPanelLowerLabel);
            this.flowLayoutPanel1.Controls.Add(this.theTopPanelLowerLabelRight);
            this.flowLayoutPanel1.Location = new System.Drawing.Point(10, 29);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(436, 23);
            this.flowLayoutPanel1.TabIndex = 0;
            this.flowLayoutPanel1.WrapContents = false;
            // 
            // theTopPanelLowerLabel
            // 
            this.theTopPanelLowerLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.theTopPanelLowerLabel.AutoEllipsis = true;
            this.theTopPanelLowerLabel.AutoSize = true;
            this.theTopPanelLowerLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelLowerLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabel.Location = new System.Drawing.Point(0, 5);
            this.theTopPanelLowerLabel.Margin = new System.Windows.Forms.Padding(0, 5, 3, 0);
            this.theTopPanelLowerLabel.Name = "theTopPanelLowerLabel";
            this.theTopPanelLowerLabel.Size = new System.Drawing.Size(158, 13);
            this.theTopPanelLowerLabel.TabIndex = 2;
            this.theTopPanelLowerLabel.Text = "<theTopPanelLowerLabel>";
            this.theTopPanelLowerLabel.UseMnemonic = false;
            // 
            // theTopPanelLowerLabelRight
            // 
            this.theTopPanelLowerLabelRight.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.theTopPanelLowerLabelRight.AutoSize = true;
            this.theTopPanelLowerLabelRight.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Underline))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabelRight.ForeColor = System.Drawing.SystemColors.Highlight;
            this.theTopPanelLowerLabelRight.Location = new System.Drawing.Point(161, 5);
            this.theTopPanelLowerLabelRight.Margin = new System.Windows.Forms.Padding(0, 5, 3, 0);
            this.theTopPanelLowerLabelRight.Name = "theTopPanelLowerLabelRight";
            this.theTopPanelLowerLabelRight.Size = new System.Drawing.Size(188, 13);
            this.theTopPanelLowerLabelRight.TabIndex = 3;
            this.theTopPanelLowerLabelRight.Text = "<theTopPanelLowerLabelRight>";
            // 
            // theParentButton
            // 
            this.theParentButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.theParentButton.Location = new System.Drawing.Point(451, 29);
            this.theParentButton.Name = "theParentButton";
            this.theParentButton.Size = new System.Drawing.Size(75, 23);
            this.theParentButton.TabIndex = 0;
            this.theParentButton.Text = "Parent";
            this.theParentButton.UseVisualStyleBackColor = true;
            this.theParentButton.Click += new System.EventHandler(this.theParentButton_Click);
            // 
            // thePreviousButton
            // 
            this.thePreviousButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.thePreviousButton.Location = new System.Drawing.Point(532, 29);
            this.thePreviousButton.Name = "thePreviousButton";
            this.thePreviousButton.Size = new System.Drawing.Size(75, 23);
            this.thePreviousButton.TabIndex = 1;
            this.thePreviousButton.Text = "Previous";
            this.thePreviousButton.UseVisualStyleBackColor = true;
            this.thePreviousButton.Click += new System.EventHandler(this.thePreviousButton_Click);
            // 
            // theNextButton
            // 
            this.theNextButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.theNextButton.Location = new System.Drawing.Point(613, 29);
            this.theNextButton.Name = "theNextButton";
            this.theNextButton.Size = new System.Drawing.Size(75, 23);
            this.theNextButton.TabIndex = 2;
            this.theNextButton.Text = "Next";
            this.theNextButton.UseVisualStyleBackColor = true;
            this.theNextButton.Click += new System.EventHandler(this.theNextButton_Click);
            // 
            // theTopPanelUpperLabel
            // 
            this.theTopPanelUpperLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.theTopPanelUpperLabel.AutoEllipsis = true;
            this.theTopPanelUpperLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelUpperLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelUpperLabel.Location = new System.Drawing.Point(10, 10);
            this.theTopPanelUpperLabel.Name = "theTopPanelUpperLabel";
            this.theTopPanelUpperLabel.Size = new System.Drawing.Size(675, 13);
            this.theTopPanelUpperLabel.TabIndex = 1;
            this.theTopPanelUpperLabel.Text = "<theTopPanelUpperLabel>";
            this.theTopPanelUpperLabel.UseMnemonic = false;
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.Controls.Add(this._configurationPanel);
            this.oneGuiPanel1.Controls.Add(this.theTopPanel);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(697, 446);
            this.oneGuiPanel1.TabIndex = 3;
            // 
            // WmsConfigurationUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "WmsConfigurationUserControl";
            this.Size = new System.Drawing.Size(697, 446);
            this._configurationPanel.ResumeLayout(false);
            this.theTopPanel.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.oneGuiPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _configurationTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _configurationPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theTopPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel theTopPanelLowerLabel;
        private System.Windows.Forms.Label theTopPanelLowerLabelRight;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theParentButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton thePreviousButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theNextButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel theTopPanelUpperLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
    }
}
