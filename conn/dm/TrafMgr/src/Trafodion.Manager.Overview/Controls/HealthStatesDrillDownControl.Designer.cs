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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class HealthStatesDrillDownControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(HealthStatesDrillDownControl));
            this._theProgressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theContentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._helpButton = new System.Windows.Forms.ToolStripButton();
            this.TrafodionLabelSubjectArea = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theContentPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theGrid)).BeginInit();
            this.TrafodionPanel1.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theProgressPanel
            // 
            this._theProgressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theProgressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theProgressPanel.Location = new System.Drawing.Point(0, 0);
            this._theProgressPanel.Name = "_theProgressPanel";
            this._theProgressPanel.Size = new System.Drawing.Size(578, 96);
            this._theProgressPanel.TabIndex = 0;
            // 
            // _theContentPanel
            // 
            this._theContentPanel.AutoScroll = true;
            this._theContentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theContentPanel.Controls.Add(this._theGrid);
            this._theContentPanel.Controls.Add(this.TrafodionPanel1);
            this._theContentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theContentPanel.Location = new System.Drawing.Point(0, 96);
            this._theContentPanel.Name = "_theContentPanel";
            this._theContentPanel.Size = new System.Drawing.Size(578, 408);
            this._theContentPanel.TabIndex = 1;
            // 
            // _theGrid
            // 
            this._theGrid.AllowColumnFilter = true;
            this._theGrid.AllowWordWrap = false;
            this._theGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("_theGrid.AlwaysHiddenColumnNames")));
            this._theGrid.AutoResizeCols = true;
            this._theGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._theGrid.CurrentFilter = null;
            this._theGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theGrid.Header.Height = 20;
            this._theGrid.HelpTopic = "";
            this._theGrid.Location = new System.Drawing.Point(0, 48);
            this._theGrid.Name = "_theGrid";
            this._theGrid.ReadOnly = true;
            this._theGrid.RowMode = true;
            this._theGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this._theGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this._theGrid.SearchAsType.SearchCol = null;
            this._theGrid.Size = new System.Drawing.Size(578, 360);
            this._theGrid.TabIndex = 5;
            this._theGrid.TreeCol = null;
            this._theGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._theGrid.WordWrap = false;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.TrafodionToolStrip1);
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabelSubjectArea);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(578, 48);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Dock = System.Windows.Forms.DockStyle.Right;
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._helpButton});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(554, 0);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(24, 48);
            this.TrafodionToolStrip1.TabIndex = 2;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // _helpButton
            // 
            this._helpButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._helpButton.Image = ((System.Drawing.Image)(resources.GetObject("_helpButton.Image")));
            this._helpButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(21, 20);
            this._helpButton.Text = "Help";
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // TrafodionLabelSubjectArea
            // 
            this.TrafodionLabelSubjectArea.AutoSize = true;
            this.TrafodionLabelSubjectArea.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TrafodionLabelSubjectArea.Location = new System.Drawing.Point(4, 12);
            this.TrafodionLabelSubjectArea.Name = "TrafodionLabelSubjectArea";
            this.TrafodionLabelSubjectArea.Size = new System.Drawing.Size(169, 16);
            this.TrafodionLabelSubjectArea.TabIndex = 1;
            this.TrafodionLabelSubjectArea.Text = "Health/State Details for ";
            // 
            // HealthStatesDrillDownControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(578, 504);
            this.Controls.Add(this._theContentPanel);
            this.Controls.Add(this._theProgressPanel);
            this.MinimizeBox = false;
            this.Name = "HealthStatesDrillDownControl";
            this.Text = "HP Database Manager - ";
            this._theContentPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._theGrid)).EndInit();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _theProgressPanel;
        private Framework.Controls.TrafodionPanel _theContentPanel;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionLabel TrafodionLabelSubjectArea;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _helpButton;
        private Framework.Controls.TrafodionIGrid _theGrid;
    }
}