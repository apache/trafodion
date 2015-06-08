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
using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class DatabaseObjectDetailsControl
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
            this.gridviewGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.theTabControlPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.dataGridviewButtonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiPanel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theProgressStatus = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.WorkingProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.theTopPanelLowerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.gridviewGroupBox.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.theTopPanel.SuspendLayout();
            this.oneGuiPanel3.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // gridviewGroupBox
            // 
            this.gridviewGroupBox.Controls.Add(this.theTabControlPanel);
            this.gridviewGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.gridviewGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.gridviewGroupBox.Location = new System.Drawing.Point(4, 26);
            this.gridviewGroupBox.Margin = new System.Windows.Forms.Padding(10);
            this.gridviewGroupBox.Name = "gridviewGroupBox";
            this.gridviewGroupBox.Size = new System.Drawing.Size(1016, 714);
            this.gridviewGroupBox.TabIndex = 6;
            this.gridviewGroupBox.TabStop = false;
            this.gridviewGroupBox.Text = "SQL Space Usage on Dec 12, 2008 at 12:20 pm";
            // 
            // theTabControlPanel
            // 
            this.theTabControlPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTabControlPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theTabControlPanel.Location = new System.Drawing.Point(3, 17);
            this.theTabControlPanel.Name = "theTabControlPanel";
            this.theTabControlPanel.Size = new System.Drawing.Size(1010, 694);
            this.theTabControlPanel.TabIndex = 4;
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.dataGridviewButtonsPanel);
            this.oneGuiPanel1.Controls.Add(this.flowLayoutPanel2);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel1.Location = new System.Drawing.Point(4, 740);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(1016, 30);
            this.oneGuiPanel1.TabIndex = 5;
            // 
            // dataGridviewButtonsPanel
            // 
            this.dataGridviewButtonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.dataGridviewButtonsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dataGridviewButtonsPanel.Location = new System.Drawing.Point(269, 0);
            this.dataGridviewButtonsPanel.Name = "dataGridviewButtonsPanel";
            this.dataGridviewButtonsPanel.Size = new System.Drawing.Size(747, 30);
            this.dataGridviewButtonsPanel.TabIndex = 1;
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Controls.Add(this.refreshButton);
            this.flowLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Left;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(269, 30);
            this.flowLayoutPanel2.TabIndex = 0;
            // 
            // refreshButton
            // 
            this.refreshButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshButton.Location = new System.Drawing.Point(3, 3);
            this.refreshButton.Name = "refreshButton";
            this.refreshButton.Size = new System.Drawing.Size(99, 23);
            this.refreshButton.TabIndex = 0;
            this.refreshButton.Text = "&Fetch Live Data";
            this.refreshButton.UseVisualStyleBackColor = true;
            this.refreshButton.Click += new System.EventHandler(this.refreshButton_Click);
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.Controls.Add(this.oneGuiPanel3);
            this.theTopPanel.Controls.Add(this.flowLayoutPanel1);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(4, 4);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(1016, 22);
            this.theTopPanel.TabIndex = 3;
            // 
            // oneGuiPanel3
            // 
            this.oneGuiPanel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel3.Controls.Add(this._theProgressStatus);
            this.oneGuiPanel3.Controls.Add(this.WorkingProgressBar);
            this.oneGuiPanel3.Dock = System.Windows.Forms.DockStyle.Right;
            this.oneGuiPanel3.Location = new System.Drawing.Point(591, 0);
            this.oneGuiPanel3.Name = "oneGuiPanel3";
            this.oneGuiPanel3.Size = new System.Drawing.Size(425, 22);
            this.oneGuiPanel3.TabIndex = 3;
            // 
            // _theProgressStatus
            // 
            this._theProgressStatus.AutoSize = true;
            this._theProgressStatus.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theProgressStatus.Location = new System.Drawing.Point(24, 4);
            this._theProgressStatus.Name = "_theProgressStatus";
            this._theProgressStatus.Size = new System.Drawing.Size(88, 13);
            this._theProgressStatus.TabIndex = 2;
            this._theProgressStatus.Text = "Fetching data ...";
            // 
            // WorkingProgressBar
            // 
            this.WorkingProgressBar.Location = new System.Drawing.Point(114, 7);
            this.WorkingProgressBar.Name = "WorkingProgressBar";
            this.WorkingProgressBar.Size = new System.Drawing.Size(292, 11);
            this.WorkingProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.WorkingProgressBar.TabIndex = 1;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.theTopPanelLowerLabel);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Left;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(432, 22);
            this.flowLayoutPanel1.TabIndex = 0;
            this.flowLayoutPanel1.WrapContents = false;
            // 
            // theTopPanelLowerLabel
            // 
            this.theTopPanelLowerLabel.AutoEllipsis = true;
            this.theTopPanelLowerLabel.AutoSize = true;
            this.theTopPanelLowerLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelLowerLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabel.Location = new System.Drawing.Point(1, 5);
            this.theTopPanelLowerLabel.Margin = new System.Windows.Forms.Padding(1, 5, 3, 0);
            this.theTopPanelLowerLabel.Name = "theTopPanelLowerLabel";
            this.theTopPanelLowerLabel.Size = new System.Drawing.Size(160, 13);
            this.theTopPanelLowerLabel.TabIndex = 2;
            this.theTopPanelLowerLabel.Text = "<theTopPanelLowerLabel>";
            this.theTopPanelLowerLabel.UseMnemonic = false;
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // DatabaseObjectDetailsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.gridviewGroupBox);
            this.Controls.Add(this.oneGuiPanel1);
            this.Controls.Add(this.theTopPanel);
            this.Name = "DatabaseObjectDetailsControl";
            this.Padding = new System.Windows.Forms.Padding(4);
            this.Size = new System.Drawing.Size(1024, 774);
            this.gridviewGroupBox.ResumeLayout(false);
            this.oneGuiPanel1.ResumeLayout(false);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.theTopPanel.ResumeLayout(false);
            this.oneGuiPanel3.ResumeLayout(false);
            this.oneGuiPanel3.PerformLayout();
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionLabel theTopPanelLowerLabel;
        private TrafodionPanel theTopPanel;
        private TrafodionPanel theTabControlPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private TrafodionPanel oneGuiPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private TrafodionButton refreshButton;
        private TrafodionPanel dataGridviewButtonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox gridviewGroupBox;
        private TrafodionProgressBar WorkingProgressBar;
        private TrafodionLabel _theProgressStatus;
        private TrafodionPanel oneGuiPanel3;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _toolTip;

    }
}
