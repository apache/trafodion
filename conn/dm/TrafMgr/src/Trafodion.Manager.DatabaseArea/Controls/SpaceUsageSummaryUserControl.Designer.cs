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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class SpaceUsageSummaryUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SpaceUsageSummaryUserControl));
            this._layoutToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._resetLayoutToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._lockLayoutToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._theDisplayPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theChartPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTopNPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._topNumericUpDown = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.topLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.spaceUsersLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.widgetCanvas = new Trafodion.Manager.Framework.WidgetCanvas();
            this._contentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._layoutToolStrip.SuspendLayout();
            this._theDisplayPanel.SuspendLayout();
            this._theTopNPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._topNumericUpDown)).BeginInit();
            this._contentPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _layoutToolStrip
            // 
            this._layoutToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._resetLayoutToolStripButton,
            this._lockLayoutToolStripButton});
            this._layoutToolStrip.Location = new System.Drawing.Point(0, 0);
            this._layoutToolStrip.Name = "_layoutToolStrip";
            this._layoutToolStrip.Size = new System.Drawing.Size(754, 25);
            this._layoutToolStrip.TabIndex = 1;
            this._layoutToolStrip.Text = "TrafodionToolStrip1";
            // 
            // _resetLayoutToolStripButton
            // 
            this._resetLayoutToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_resetLayoutToolStripButton.Image")));
            this._resetLayoutToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._resetLayoutToolStripButton.Name = "_resetLayoutToolStripButton";
            this._resetLayoutToolStripButton.Size = new System.Drawing.Size(94, 22);
            this._resetLayoutToolStripButton.Text = "Reset Layout";
            this._resetLayoutToolStripButton.Click += new System.EventHandler(this._resetLayoutToolStripButton_Click);
            // 
            // _lockLayoutToolStripButton
            // 
            this._lockLayoutToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_lockLayoutToolStripButton.Image")));
            this._lockLayoutToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._lockLayoutToolStripButton.Name = "_lockLayoutToolStripButton";
            this._lockLayoutToolStripButton.Size = new System.Drawing.Size(91, 22);
            this._lockLayoutToolStripButton.Text = "Lock Layout";
            this._lockLayoutToolStripButton.Click += new System.EventHandler(this._lockLayoutToolStripButton_Click);
            // 
            // _theDisplayPanel
            // 
            this._theDisplayPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theDisplayPanel.Controls.Add(this._theChartPanel);
            this._theDisplayPanel.Controls.Add(this._theTopNPanel);
            this._theDisplayPanel.Location = new System.Drawing.Point(3, 235);
            this._theDisplayPanel.Name = "_theDisplayPanel";
            this._theDisplayPanel.Size = new System.Drawing.Size(688, 146);
            this._theDisplayPanel.TabIndex = 0;
            // 
            // _theChartPanel
            // 
            this._theChartPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theChartPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theChartPanel.Location = new System.Drawing.Point(0, 41);
            this._theChartPanel.Name = "_theChartPanel";
            this._theChartPanel.Size = new System.Drawing.Size(688, 105);
            this._theChartPanel.TabIndex = 3;
            // 
            // _theTopNPanel
            // 
            this._theTopNPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTopNPanel.Controls.Add(this._topNumericUpDown);
            this._theTopNPanel.Controls.Add(this.topLabel);
            this._theTopNPanel.Controls.Add(this.spaceUsersLabel);
            this._theTopNPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theTopNPanel.Location = new System.Drawing.Point(0, 0);
            this._theTopNPanel.Name = "_theTopNPanel";
            this._theTopNPanel.Size = new System.Drawing.Size(688, 41);
            this._theTopNPanel.TabIndex = 2;
            // 
            // _topNumericUpDown
            // 
            this._topNumericUpDown.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._topNumericUpDown.Location = new System.Drawing.Point(42, 8);
            this._topNumericUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this._topNumericUpDown.Name = "_topNumericUpDown";
            this._topNumericUpDown.Size = new System.Drawing.Size(44, 23);
            this._topNumericUpDown.TabIndex = 1;
            this._topNumericUpDown.Value = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this._topNumericUpDown.ValueChanged += new System.EventHandler(this._topNumericUpDown_ValueChanged);
            // 
            // topLabel
            // 
            this.topLabel.AutoSize = true;
            this.topLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.topLabel.Location = new System.Drawing.Point(9, 10);
            this.topLabel.Name = "topLabel";
            this.topLabel.Size = new System.Drawing.Size(31, 16);
            this.topLabel.TabIndex = 0;
            this.topLabel.Text = "Top";
            // 
            // spaceUsersLabel
            // 
            this.spaceUsersLabel.AutoSize = true;
            this.spaceUsersLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.spaceUsersLabel.Location = new System.Drawing.Point(92, 10);
            this.spaceUsersLabel.Name = "spaceUsersLabel";
            this.spaceUsersLabel.Size = new System.Drawing.Size(87, 16);
            this.spaceUsersLabel.TabIndex = 0;
            this.spaceUsersLabel.Text = "Space Users";
            // 
            // widgetCanvas
            // 
            this.widgetCanvas.ActiveWidget = null;
            this.widgetCanvas.AllowDelete = true;
            this.widgetCanvas.AllowDrop = true;
            this.widgetCanvas.LayoutManager = null;
            this.widgetCanvas.Location = new System.Drawing.Point(3, 3);
            this.widgetCanvas.LockBackColor = System.Drawing.SystemColors.Control;
            this.widgetCanvas.Name = "widgetCanvas";
            this.widgetCanvas.Size = new System.Drawing.Size(637, 198);
            this.widgetCanvas.TabIndex = 0;
            this.widgetCanvas.ThePersistenceKey = null;
            this.widgetCanvas.UnlockBackColor = System.Drawing.Color.Azure;
            this.widgetCanvas.ViewName = null;
            this.widgetCanvas.ViewNum = 0;
            this.widgetCanvas.ViewText = null;
            this.widgetCanvas.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("widgetCanvas.WidgetsModel")));
            // 
            // _contentPanel
            // 
            this._contentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._contentPanel.Controls.Add(this.widgetCanvas);
            this._contentPanel.Controls.Add(this._theDisplayPanel);
            this._contentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._contentPanel.Location = new System.Drawing.Point(0, 25);
            this._contentPanel.Name = "_contentPanel";
            this._contentPanel.Size = new System.Drawing.Size(754, 384);
            this._contentPanel.TabIndex = 2;
            // 
            // SpaceUsageSummaryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._contentPanel);
            this.Controls.Add(this._layoutToolStrip);
            this.Name = "SpaceUsageSummaryUserControl";
            this.Size = new System.Drawing.Size(754, 409);
            this.Load += new System.EventHandler(this.SpaceUsageSummaryUserControl_Load);
            this._layoutToolStrip.ResumeLayout(false);
            this._layoutToolStrip.PerformLayout();
            this._theDisplayPanel.ResumeLayout(false);
            this._theTopNPanel.ResumeLayout(false);
            this._theTopNPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._topNumericUpDown)).EndInit();
            this._contentPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionPanel _theDisplayPanel;
        private Framework.WidgetCanvas widgetCanvas;
        private Framework.Controls.TrafodionPanel _theChartPanel;
        private Framework.Controls.TrafodionPanel _theTopNPanel;
        private Framework.Controls.TrafodionNumericUpDown _topNumericUpDown;
        private Framework.Controls.TrafodionLabel topLabel;
        private Framework.Controls.TrafodionLabel spaceUsersLabel;
        private Framework.Controls.TrafodionToolStrip _layoutToolStrip;
        private System.Windows.Forms.ToolStripButton _resetLayoutToolStripButton;
        private System.Windows.Forms.ToolStripButton _lockLayoutToolStripButton;
        private Framework.Controls.TrafodionPanel _contentPanel;
    }
}
