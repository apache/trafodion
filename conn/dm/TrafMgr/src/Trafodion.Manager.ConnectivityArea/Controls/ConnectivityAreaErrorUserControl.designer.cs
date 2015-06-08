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
using System.Windows.Forms;
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaErrorUserControl
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
            this._connectivityErrorSubTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._ConnectivityMainBodyPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.theTopPanelLowerLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.theTopPanelLowerLabelRight = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._ConnectivityMainBodyPanel.SuspendLayout();
            this._bottomPanel.SuspendLayout();
            this.theTopPanel.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _connectivityErrorSubTabControl
            // 
            this._connectivityErrorSubTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._connectivityErrorSubTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._connectivityErrorSubTabControl.Location = new System.Drawing.Point(0, 0);
            this._connectivityErrorSubTabControl.Multiline = true;
            this._connectivityErrorSubTabControl.Name = "_connectivityErrorSubTabControl";
            this._connectivityErrorSubTabControl.Padding = new System.Drawing.Point(10, 5);
            this._connectivityErrorSubTabControl.SelectedIndex = 0;
            this._connectivityErrorSubTabControl.Size = new System.Drawing.Size(697, 376);
            this._connectivityErrorSubTabControl.TabIndex = 0;
            this._connectivityErrorSubTabControl.Visible = false;
            // 
            // _ConnectivityMainBodyPanel
            // 
            this._ConnectivityMainBodyPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._ConnectivityMainBodyPanel.Controls.Add(this._connectivityErrorSubTabControl);
            this._ConnectivityMainBodyPanel.Controls.Add(this._bottomPanel);
            this._ConnectivityMainBodyPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._ConnectivityMainBodyPanel.Location = new System.Drawing.Point(0, 37);
            this._ConnectivityMainBodyPanel.Name = "_ConnectivityMainBodyPanel";
            this._ConnectivityMainBodyPanel.Size = new System.Drawing.Size(697, 409);
            this._ConnectivityMainBodyPanel.TabIndex = 1;
            // 
            // _bottomPanel
            // 
            this._bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._bottomPanel.Controls.Add(this._refreshButton);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 376);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.Size = new System.Drawing.Size(697, 33);
            this._bottomPanel.TabIndex = 6;
            // 
            // _refreshButton
            // 
            this._refreshButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._refreshButton.Location = new System.Drawing.Point(3, 5);
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(79, 23);
            this._refreshButton.TabIndex = 0;
            this._refreshButton.Text = "&Refresh";
            this._refreshButton.UseVisualStyleBackColor = true;
            this._refreshButton.Click += new System.EventHandler(this._refreshButton_Click);
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.theTopPanel.Controls.Add(this.flowLayoutPanel1);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(697, 37);
            this.theTopPanel.TabIndex = 4;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.theTopPanelLowerLabel);
            this.flowLayoutPanel1.Controls.Add(this.theTopPanelLowerLabelRight);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(693, 33);
            this.flowLayoutPanel1.TabIndex = 0;
            this.flowLayoutPanel1.WrapContents = false;
            // 
            // theTopPanelLowerLabel
            // 
            this.theTopPanelLowerLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.theTopPanelLowerLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelLowerLabel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theTopPanelLowerLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabel.Location = new System.Drawing.Point(0, 5);
            this.theTopPanelLowerLabel.Margin = new System.Windows.Forms.Padding(0, 5, 3, 0);
            this.theTopPanelLowerLabel.Name = "theTopPanelLowerLabel";
            this.theTopPanelLowerLabel.Size = new System.Drawing.Size(174, 15);
            this.theTopPanelLowerLabel.TabIndex = 2;
            this.theTopPanelLowerLabel.Text = "<theTopPanelLowerLabel>";
            // 
            // theTopPanelLowerLabelRight
            // 
            this.theTopPanelLowerLabelRight.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.theTopPanelLowerLabelRight.AutoSize = true;
            this.theTopPanelLowerLabelRight.Font = new System.Drawing.Font("Tahoma", 9F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Underline))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabelRight.ForeColor = System.Drawing.SystemColors.Highlight;
            this.theTopPanelLowerLabelRight.Location = new System.Drawing.Point(177, 13);
            this.theTopPanelLowerLabelRight.Margin = new System.Windows.Forms.Padding(0, 5, 3, 0);
            this.theTopPanelLowerLabelRight.Name = "theTopPanelLowerLabelRight";
            this.theTopPanelLowerLabelRight.Size = new System.Drawing.Size(208, 14);
            this.theTopPanelLowerLabelRight.TabIndex = 3;
            this.theTopPanelLowerLabelRight.Text = "<theTopPanelLowerLabelRight>";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._ConnectivityMainBodyPanel);
            this.TrafodionPanel1.Controls.Add(this.theTopPanel);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(697, 446);
            this.TrafodionPanel1.TabIndex = 3;
            // 
            // ConnectivityAreaErrorUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaErrorUserControl";
            this.Size = new System.Drawing.Size(697, 446);
            this._ConnectivityMainBodyPanel.ResumeLayout(false);
            this._bottomPanel.ResumeLayout(false);
            this.theTopPanel.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _connectivityErrorSubTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _ConnectivityMainBodyPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theTopPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox theTopPanelLowerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel theTopPanelLowerLabelRight;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _refreshButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
    }
}
