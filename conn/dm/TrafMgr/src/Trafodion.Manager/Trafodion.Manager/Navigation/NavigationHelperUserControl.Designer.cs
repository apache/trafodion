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
﻿namespace Trafodion.Manager.Framework.Navigation
{
    partial class NavigationHelperUserControl
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
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanelLowerLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.theTopPanelLowerLabelRight = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.theParentButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.thePreviousButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theNextButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theTopPanelUpperLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.theTopPanel.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.Controls.Add(this.TrafodionPanel2);
            this.theTopPanel.Controls.Add(this.theParentButton);
            this.theTopPanel.Controls.Add(this.thePreviousButton);
            this.theTopPanel.Controls.Add(this.theNextButton);
            this.theTopPanel.Controls.Add(this.theTopPanelUpperLabel);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(916, 65);
            this.theTopPanel.TabIndex = 5;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this.theTopPanelLowerLabel);
            this.TrafodionPanel2.Controls.Add(this.theTopPanelLowerLabelRight);
            this.TrafodionPanel2.Location = new System.Drawing.Point(10, 32);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(654, 23);
            this.TrafodionPanel2.TabIndex = 4;
            // 
            // theTopPanelLowerLabel
            // 
            this.theTopPanelLowerLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelLowerLabel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theTopPanelLowerLabel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theTopPanelLowerLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanelLowerLabel.Margin = new System.Windows.Forms.Padding(0, 5, 3, 0);
            this.theTopPanelLowerLabel.Name = "theTopPanelLowerLabel";
            this.theTopPanelLowerLabel.ReadOnly = true;
            this.theTopPanelLowerLabel.Size = new System.Drawing.Size(464, 14);
            this.theTopPanelLowerLabel.TabIndex = 2;
            this.theTopPanelLowerLabel.Text = "<theTopPanelLowerLabel>";
            // 
            // theTopPanelLowerLabelRight
            // 
            this.theTopPanelLowerLabelRight.AutoEllipsis = true;
            this.theTopPanelLowerLabelRight.AutoSize = true;
            this.theTopPanelLowerLabelRight.Dock = System.Windows.Forms.DockStyle.Right;
            this.theTopPanelLowerLabelRight.Font = new System.Drawing.Font("Tahoma", 8.25F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Underline))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabelRight.ForeColor = System.Drawing.SystemColors.Highlight;
            this.theTopPanelLowerLabelRight.Location = new System.Drawing.Point(464, 0);
            this.theTopPanelLowerLabelRight.Margin = new System.Windows.Forms.Padding(0, 5, 3, 0);
            this.theTopPanelLowerLabelRight.Name = "theTopPanelLowerLabelRight";
            this.theTopPanelLowerLabelRight.Size = new System.Drawing.Size(190, 13);
            this.theTopPanelLowerLabelRight.TabIndex = 3;
            this.theTopPanelLowerLabelRight.Text = "<theTopPanelLowerLabelRight>";
            this.theTopPanelLowerLabelRight.Click += new System.EventHandler(this.theTopPanelLowerLabelRight_Click);
            // 
            // theParentButton
            // 
            this.theParentButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.theParentButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.theParentButton.Image = global::Trafodion.Manager.Properties.Resources.UpArrowIcon;
            this.theParentButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.theParentButton.Location = new System.Drawing.Point(670, 33);
            this.theParentButton.Name = "theParentButton";
            this.theParentButton.Size = new System.Drawing.Size(75, 23);
            this.theParentButton.TabIndex = 0;
            this.theParentButton.Text = "Parent";
            this.theParentButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.theParentButton.UseVisualStyleBackColor = true;
            this.theParentButton.Click += new System.EventHandler(this.theParentButton_Click);
            // 
            // thePreviousButton
            // 
            this.thePreviousButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.thePreviousButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.thePreviousButton.Image = global::Trafodion.Manager.Properties.Resources.LeftArrowIcon;
            this.thePreviousButton.Location = new System.Drawing.Point(751, 33);
            this.thePreviousButton.Name = "thePreviousButton";
            this.thePreviousButton.Size = new System.Drawing.Size(75, 23);
            this.thePreviousButton.TabIndex = 1;
            this.thePreviousButton.Text = "Previous";
            this.thePreviousButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.thePreviousButton.UseVisualStyleBackColor = true;
            this.thePreviousButton.Click += new System.EventHandler(this.thePreviousButton_Click);
            // 
            // theNextButton
            // 
            this.theNextButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.theNextButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.theNextButton.Image = global::Trafodion.Manager.Properties.Resources.RightArrowIcon;
            this.theNextButton.Location = new System.Drawing.Point(832, 33);
            this.theNextButton.Name = "theNextButton";
            this.theNextButton.Size = new System.Drawing.Size(75, 23);
            this.theNextButton.TabIndex = 2;
            this.theNextButton.Text = "Next";
            this.theNextButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.theNextButton.UseVisualStyleBackColor = true;
            this.theNextButton.Click += new System.EventHandler(this.theNextButton_Click);
            // 
            // theTopPanelUpperLabel
            // 
            this.theTopPanelUpperLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.theTopPanelUpperLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelUpperLabel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theTopPanelUpperLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelUpperLabel.Location = new System.Drawing.Point(10, 10);
            this.theTopPanelUpperLabel.Name = "theTopPanelUpperLabel";
            this.theTopPanelUpperLabel.ReadOnly = true;
            this.theTopPanelUpperLabel.Size = new System.Drawing.Size(894, 14);
            this.theTopPanelUpperLabel.TabIndex = 1;
            this.theTopPanelUpperLabel.Text = "<theTopPanelUpperLabel>";
            // 
            // NavigationHelperUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.theTopPanel);
            this.Name = "NavigationHelperUserControl";
            this.Size = new System.Drawing.Size(916, 65);
            this.theTopPanel.ResumeLayout(false);
            this.theTopPanel.PerformLayout();
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel theTopPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox theTopPanelLowerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel theTopPanelLowerLabelRight;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theParentButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton thePreviousButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theNextButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox theTopPanelUpperLabel;
    }
}
