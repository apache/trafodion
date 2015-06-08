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
namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class ServicesSummaryUserControl
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
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._exportButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._releaseAllButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._holdAllButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._holdButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._releaseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._stopButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._deleteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._alterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._addButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._startButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._bottomPanel.SuspendLayout();
            this._theAdditionalButtonPanel.SuspendLayout();
            this.SuspendLayout();

            //Context Menu Items
            this.releaseAllServicesMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.holdAllServicesMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.holdServiceMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.releaseServiceMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.stopServiceMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.deleteServiceMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.alterServiceMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.addServiceMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();
            this.startServiceMenuItem = new Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem();            
            
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 0);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(918, 424);
            this._theWidgetPanel.TabIndex = 3;
            // 
            // _bottomPanel
            // 
            this._bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomPanel.Controls.Add(this._theAdditionalButtonPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 424);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.Size = new System.Drawing.Size(918, 33);
            this._bottomPanel.TabIndex = 2;
            // 
            // _theAdditionalButtonPanel
            // 
            this._theAdditionalButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalButtonPanel.Controls.Add(this._exportButton);
            this._theAdditionalButtonPanel.Controls.Add(this._releaseAllButton);
            this._theAdditionalButtonPanel.Controls.Add(this._holdAllButton);
            this._theAdditionalButtonPanel.Controls.Add(this._holdButton);
            this._theAdditionalButtonPanel.Controls.Add(this._releaseButton);
            this._theAdditionalButtonPanel.Controls.Add(this._stopButton);
            this._theAdditionalButtonPanel.Controls.Add(this._deleteButton);
            this._theAdditionalButtonPanel.Controls.Add(this._alterButton);
            this._theAdditionalButtonPanel.Controls.Add(this._addButton);
            this._theAdditionalButtonPanel.Controls.Add(this._startButton);
            this._theAdditionalButtonPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalButtonPanel.Location = new System.Drawing.Point(0, 0);
            this._theAdditionalButtonPanel.Name = "_theAdditionalButtonPanel";
            this._theAdditionalButtonPanel.Size = new System.Drawing.Size(918, 33);
            this._theAdditionalButtonPanel.TabIndex = 1;
            // 
            // _exportButton
            // 
            this._exportButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._exportButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._exportButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._exportButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._exportButton.Location = new System.Drawing.Point(835, 4);
            this._exportButton.Name = "_exportButton";
            this._exportButton.Size = new System.Drawing.Size(75, 23);
            this._exportButton.TabIndex = 3;
            this._exportButton.Text = "&Export";
            this._exportButton.UseVisualStyleBackColor = true;
            this._exportButton.Click += new System.EventHandler(this._exportButton_Click);
            // 
            // _releaseAllButton
            // 
            this._releaseAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._releaseAllButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._releaseAllButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._releaseAllButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._releaseAllButton.Location = new System.Drawing.Point(722, 4);
            this._releaseAllButton.Name = "_releaseAllButton";
            this._releaseAllButton.Size = new System.Drawing.Size(75, 23);
            this._releaseAllButton.TabIndex = 3;
            this._releaseAllButton.Text = "R&elease All";
            this._releaseAllButton.UseVisualStyleBackColor = true;
            this._releaseAllButton.Click += new System.EventHandler(this._releaseAllButton_Click);
            // 
            // _holdAllButton
            // 
            this._holdAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._holdAllButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._holdAllButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._holdAllButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._holdAllButton.Location = new System.Drawing.Point(641, 4);
            this._holdAllButton.Name = "_holdAllButton";
            this._holdAllButton.Size = new System.Drawing.Size(75, 23);
            this._holdAllButton.TabIndex = 3;
            this._holdAllButton.Text = "H&old All";
            this._holdAllButton.UseVisualStyleBackColor = true;
            this._holdAllButton.Click += new System.EventHandler(this._holdAllButton_Click);
            // 
            // _holdButton
            // 
            this._holdButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._holdButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._holdButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._holdButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._holdButton.Location = new System.Drawing.Point(479, 4);
            this._holdButton.Name = "_holdButton";
            this._holdButton.Size = new System.Drawing.Size(75, 23);
            this._holdButton.TabIndex = 2;
            this._holdButton.Text = "&Hold";
            this._holdButton.UseVisualStyleBackColor = true;
            this._holdButton.Click += new System.EventHandler(this.holdButton_Click);
            // 
            // _releaseButton
            // 
            this._releaseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._releaseButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._releaseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._releaseButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._releaseButton.Location = new System.Drawing.Point(560, 4);
            this._releaseButton.Name = "_releaseButton";
            this._releaseButton.Size = new System.Drawing.Size(75, 23);
            this._releaseButton.TabIndex = 2;
            this._releaseButton.Text = "&Release";
            this._releaseButton.UseVisualStyleBackColor = true;
            this._releaseButton.Click += new System.EventHandler(this._releaseButton_Click);
            // 
            // _stopButton
            // 
            this._stopButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._stopButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._stopButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._stopButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._stopButton.Location = new System.Drawing.Point(375, 4);
            this._stopButton.Name = "_stopButton";
            this._stopButton.Size = new System.Drawing.Size(75, 23);
            this._stopButton.TabIndex = 1;
            this._stopButton.Text = "S&top";
            this._stopButton.UseVisualStyleBackColor = true;
            this._stopButton.Click += new System.EventHandler(this.stopButton_Click);
            // 
            // _deleteButton
            // 
            this._deleteButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._deleteButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._deleteButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._deleteButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._deleteButton.Location = new System.Drawing.Point(167, 4);
            this._deleteButton.Name = "_deleteButton";
            this._deleteButton.Size = new System.Drawing.Size(75, 23);
            this._deleteButton.TabIndex = 0;
            this._deleteButton.Text = "&Delete";
            this._deleteButton.UseVisualStyleBackColor = true;
            this._deleteButton.Click += new System.EventHandler(this._deleteButton_Click);
            // 
            // _alterButton
            // 
            this._alterButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._alterButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._alterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alterButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._alterButton.Location = new System.Drawing.Point(86, 4);
            this._alterButton.Name = "_alterButton";
            this._alterButton.Size = new System.Drawing.Size(75, 23);
            this._alterButton.TabIndex = 0;
            this._alterButton.Text = "A&lter";
            this._alterButton.UseVisualStyleBackColor = true;
            this._alterButton.Click += new System.EventHandler(this._alterButton_Click);
            // 
            // _addButton
            // 
            this._addButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._addButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._addButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._addButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._addButton.Location = new System.Drawing.Point(5, 4);
            this._addButton.Name = "_addButton";
            this._addButton.Size = new System.Drawing.Size(75, 23);
            this._addButton.TabIndex = 0;
            this._addButton.Text = "&Add";
            this._addButton.UseVisualStyleBackColor = true;
            this._addButton.Click += new System.EventHandler(this._addButton_Click);
            // 
            // _startButton
            // 
            this._startButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._startButton.BackColor = System.Drawing.Color.WhiteSmoke;
            this._startButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._startButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._startButton.Location = new System.Drawing.Point(294, 4);
            this._startButton.Name = "_startButton";
            this._startButton.Size = new System.Drawing.Size(75, 23);
            this._startButton.TabIndex = 0;
            this._startButton.Text = "&Start";
            this._startButton.UseVisualStyleBackColor = true;
            this._startButton.Click += new System.EventHandler(this.startButton_Click);
            // 
            // ServicesSummaryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theWidgetPanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "ServicesSummaryUserControl";
            this.Size = new System.Drawing.Size(918, 457);
            this._bottomPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _startButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _holdAllButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _releaseButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _stopButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _addButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _alterButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _releaseAllButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _holdButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _deleteButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _exportButton;

        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem addServiceMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem startServiceMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem holdServiceMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem releaseServiceMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem stopServiceMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem alterServiceMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem releaseAllServicesMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem holdAllServicesMenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionIGridToolStripMenuItem deleteServiceMenuItem;
    }
}
