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
    partial class ExpertChartDesignerUserControl
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
            this._theSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theChartsTreeView = new Trafodion.Manager.Framework.Controls.TrafodionTreeView();
            this._theTablePanel = new System.Windows.Forms.TableLayoutPanel();
            this._theAttributePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSplitContainer.Panel1.SuspendLayout();
            this._theSplitContainer.Panel2.SuspendLayout();
            this._theSplitContainer.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this._theTablePanel.SuspendLayout();
            this.TrafodionPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theSplitContainer
            // 
            this._theSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSplitContainer.Location = new System.Drawing.Point(0, 0);
            this._theSplitContainer.Name = "_theSplitContainer";
            // 
            // _theSplitContainer.Panel1
            // 
            this._theSplitContainer.Panel1.Controls.Add(this.TrafodionGroupBox1);
            // 
            // _theSplitContainer.Panel2
            // 
            this._theSplitContainer.Panel2.AutoScroll = true;
            this._theSplitContainer.Panel2.Controls.Add(this._theTablePanel);
            this._theSplitContainer.Size = new System.Drawing.Size(801, 322);
            this._theSplitContainer.SplitterDistance = 265;
            this._theSplitContainer.SplitterWidth = 9;
            this._theSplitContainer.TabIndex = 1;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._theChartsTreeView);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(265, 322);
            this.TrafodionGroupBox1.TabIndex = 0;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Chart";
            // 
            // _theChartsTreeView
            // 
            this._theChartsTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theChartsTreeView.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theChartsTreeView.ImageKey = "BLANK_DOCUMENT_ICON";
            this._theChartsTreeView.Location = new System.Drawing.Point(3, 17);
            this._theChartsTreeView.Name = "_theChartsTreeView";
            this._theChartsTreeView.SelectedImageKey = "BLANK_DOCUMENT_ICON";
            this._theChartsTreeView.Size = new System.Drawing.Size(259, 302);
            this._theChartsTreeView.TabIndex = 4;
            this._theChartsTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this._theChartsTreeView_AfterSelect);
            // 
            // _theTablePanel
            // 
            this._theTablePanel.ColumnCount = 1;
            this._theTablePanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this._theTablePanel.Controls.Add(this._theAttributePanel, 0, 0);
            this._theTablePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTablePanel.Location = new System.Drawing.Point(0, 0);
            this._theTablePanel.Name = "_theTablePanel";
            this._theTablePanel.RowCount = 1;
            this._theTablePanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 90.43928F));
            this._theTablePanel.Size = new System.Drawing.Size(527, 322);
            this._theTablePanel.TabIndex = 1;
            // 
            // _theAttributePanel
            // 
            this._theAttributePanel.AutoScroll = true;
            this._theAttributePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAttributePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAttributePanel.Location = new System.Drawing.Point(3, 3);
            this._theAttributePanel.Name = "_theAttributePanel";
            this._theAttributePanel.Size = new System.Drawing.Size(521, 316);
            this._theAttributePanel.TabIndex = 0;
            // 
            // TrafodionPanel
            // 
            this.TrafodionPanel.AutoScroll = true;
            this.TrafodionPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel.Controls.Add(this._theSplitContainer);
            this.TrafodionPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel.Name = "TrafodionPanel";
            this.TrafodionPanel.Size = new System.Drawing.Size(801, 322);
            this.TrafodionPanel.TabIndex = 2;
            // 
            // ExpertChartDesignerUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel);
            this.Name = "ExpertChartDesignerUserControl";
            this.Size = new System.Drawing.Size(801, 322);
            this._theSplitContainer.Panel1.ResumeLayout(false);
            this._theSplitContainer.Panel2.ResumeLayout(false);
            this._theSplitContainer.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this._theTablePanel.ResumeLayout(false);
            this.TrafodionPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionSplitContainer _theSplitContainer;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private System.Windows.Forms.TableLayoutPanel _theTablePanel;
        private Framework.Controls.TrafodionPanel _theAttributePanel;
        private Framework.Controls.TrafodionPanel TrafodionPanel;
        private Framework.Controls.TrafodionTreeView _theChartsTreeView;



    }
}
