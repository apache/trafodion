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
    public partial class DatabaseObjectAggregateControl
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
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.theTablelPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.tableGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiPanel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theProgressStatus = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.WorkingProgressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this.flowLayoutPanel3 = new System.Windows.Forms.FlowLayoutPanel();
            this.theTopPanelLowerLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.flowLayoutPanel2.SuspendLayout();
            this.theTablelPanel.SuspendLayout();
            this.theTopPanel.SuspendLayout();
            this.oneGuiPanel2.SuspendLayout();
            this.oneGuiPanel3.SuspendLayout();
            this.flowLayoutPanel3.SuspendLayout();
            this.SuspendLayout();
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Controls.Add(this.refreshButton);
            this.flowLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.flowLayoutPanel2.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(0, 273);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this.flowLayoutPanel2.Size = new System.Drawing.Size(860, 31);
            this.flowLayoutPanel2.TabIndex = 5;
            // 
            // theTablelPanel
            // 
            this.theTablelPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTablelPanel.Controls.Add(this.tableGroupBox);
            this.theTablelPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theTablelPanel.Location = new System.Drawing.Point(0, 24);
            this.theTablelPanel.Name = "theTablelPanel";
            this.theTablelPanel.Size = new System.Drawing.Size(860, 249);
            this.theTablelPanel.TabIndex = 4;
            // 
            // tableGroupBox
            // 
            this.tableGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.tableGroupBox.Location = new System.Drawing.Point(0, 0);
            this.tableGroupBox.Name = "tableGroupBox";
            this.tableGroupBox.Padding = new System.Windows.Forms.Padding(10);
            this.tableGroupBox.Size = new System.Drawing.Size(860, 249);
            this.tableGroupBox.TabIndex = 0;
            this.tableGroupBox.TabStop = false;
            this.tableGroupBox.Text = "Total SQL Space Usage on December 3, 2008 at 3:10 pm";
            // 
            // refreshButton
            // 
            this.refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshButton.Location = new System.Drawing.Point(3, 3);
            this.refreshButton.Name = "refreshButton";
            this.refreshButton.Size = new System.Drawing.Size(99, 23);
            this.refreshButton.TabIndex = 1;
            this.refreshButton.Text = "&Fetch Live Data";
            this.refreshButton.UseVisualStyleBackColor = true;
            this.refreshButton.Click += new System.EventHandler(this.refreshButton_Click);
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.Controls.Add(this.oneGuiPanel2);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(860, 24);
            this.theTopPanel.TabIndex = 3;
            // 
            // oneGuiPanel2
            // 
            this.oneGuiPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel2.Controls.Add(this.oneGuiPanel3);
            this.oneGuiPanel2.Controls.Add(this.flowLayoutPanel3);
            this.oneGuiPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.oneGuiPanel2.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel2.Name = "oneGuiPanel2";
            this.oneGuiPanel2.Size = new System.Drawing.Size(860, 22);
            this.oneGuiPanel2.TabIndex = 3;
            // 
            // oneGuiPanel3
            // 
            this.oneGuiPanel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel3.Controls.Add(this._theProgressStatus);
            this.oneGuiPanel3.Controls.Add(this.WorkingProgressBar);
            this.oneGuiPanel3.Dock = System.Windows.Forms.DockStyle.Right;
            this.oneGuiPanel3.Location = new System.Drawing.Point(459, 0);
            this.oneGuiPanel3.Name = "oneGuiPanel3";
            this.oneGuiPanel3.Size = new System.Drawing.Size(401, 22);
            this.oneGuiPanel3.TabIndex = 3;
            // 
            // _theProgressStatus
            // 
            this._theProgressStatus.AutoSize = true;
            this._theProgressStatus.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theProgressStatus.Location = new System.Drawing.Point(5, 4);
            this._theProgressStatus.Name = "_theProgressStatus";
            this._theProgressStatus.Size = new System.Drawing.Size(88, 13);
            this._theProgressStatus.TabIndex = 2;
            this._theProgressStatus.Text = "Fetching data ...";
            // 
            // WorkingProgressBar
            // 
            this.WorkingProgressBar.Location = new System.Drawing.Point(95, 6);
            this.WorkingProgressBar.Name = "WorkingProgressBar";
            this.WorkingProgressBar.Size = new System.Drawing.Size(292, 11);
            this.WorkingProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.WorkingProgressBar.TabIndex = 1;
            // 
            // flowLayoutPanel3
            // 
            this.flowLayoutPanel3.Controls.Add(this.theTopPanelLowerLabel);
            this.flowLayoutPanel3.Dock = System.Windows.Forms.DockStyle.Left;
            this.flowLayoutPanel3.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel3.Name = "flowLayoutPanel3";
            this.flowLayoutPanel3.Size = new System.Drawing.Size(333, 22);
            this.flowLayoutPanel3.TabIndex = 0;
            this.flowLayoutPanel3.WrapContents = false;
            // 
            // theTopPanelLowerLabel
            // 
            this.theTopPanelLowerLabel.AutoEllipsis = true;
            this.theTopPanelLowerLabel.AutoSize = true;
            this.theTopPanelLowerLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelLowerLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabel.Location = new System.Drawing.Point(0, 5);
            this.theTopPanelLowerLabel.Margin = new System.Windows.Forms.Padding(0, 5, 3, 0);
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
            // DatabaseObjectAggregateControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.theTablelPanel);
            this.Controls.Add(this.flowLayoutPanel2);
            this.Controls.Add(this.theTopPanel);
            this.Name = "DatabaseObjectAggregateControl";
            this.Size = new System.Drawing.Size(860, 304);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.theTablelPanel.ResumeLayout(false);
            this.theTopPanel.ResumeLayout(false);
            this.oneGuiPanel2.ResumeLayout(false);
            this.oneGuiPanel3.ResumeLayout(false);
            this.oneGuiPanel3.PerformLayout();
            this.flowLayoutPanel3.ResumeLayout(false);
            this.flowLayoutPanel3.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionPanel theTopPanel;
        private TrafodionPanel theTablelPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private TrafodionButton refreshButton;
        private TrafodionGroupBox tableGroupBox;
        private TrafodionPanel oneGuiPanel2;
        private TrafodionPanel oneGuiPanel3;
        private TrafodionLabel _theProgressStatus;
        private TrafodionProgressBar WorkingProgressBar;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel3;
        private TrafodionLabel theTopPanelLowerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _toolTip;

    }
}
