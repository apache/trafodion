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
    partial class WorkloadsUserControl
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
            MyDispose(disposing); 

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
            this._liveViewSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._workloadsGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._wmsSummaryUserControl = new Trafodion.Manager.WorkloadArea.Controls.WMSSummaryUserControl();
            this._liveViewSplitContainer.Panel1.SuspendLayout();
            this._liveViewSplitContainer.Panel2.SuspendLayout();
            this._liveViewSplitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // _liveViewSplitContainer
            // 
            this._liveViewSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._liveViewSplitContainer.Location = new System.Drawing.Point(0, 0);
            this._liveViewSplitContainer.Name = "_liveViewSplitContainer";
            this._liveViewSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _liveViewSplitContainer.Panel1
            // 
            this._liveViewSplitContainer.Panel1.Controls.Add(this._workloadsGridPanel);
            // 
            // _liveViewSplitContainer.Panel2
            // 
            this._liveViewSplitContainer.Panel2.Controls.Add(this._wmsSummaryUserControl);
            this._liveViewSplitContainer.Size = new System.Drawing.Size(954, 559);
            this._liveViewSplitContainer.SplitterDistance = 345;
            this._liveViewSplitContainer.SplitterWidth = 9;
            this._liveViewSplitContainer.TabIndex = 0;
            // 
            // _workloadsGridPanel
            // 
            this._workloadsGridPanel.AutoScroll = true;
            this._workloadsGridPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._workloadsGridPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._workloadsGridPanel.Location = new System.Drawing.Point(0, 0);
            this._workloadsGridPanel.Name = "_workloadsGridPanel";
            this._workloadsGridPanel.Size = new System.Drawing.Size(954, 345);
            this._workloadsGridPanel.TabIndex = 2;
            // 
            // _wmsSummaryUserControl
            // 
            this._wmsSummaryUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._wmsSummaryUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._wmsSummaryUserControl.Location = new System.Drawing.Point(0, 0);
            this._wmsSummaryUserControl.Name = "_wmsSummaryUserControl";
            this._wmsSummaryUserControl.Size = new System.Drawing.Size(954, 205);
            this._wmsSummaryUserControl.TabIndex = 0;
            // 
            // WorkloadsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._liveViewSplitContainer);
            this.Name = "WorkloadsUserControl";
            this.Size = new System.Drawing.Size(954, 559);
            this._liveViewSplitContainer.Panel1.ResumeLayout(false);
            this._liveViewSplitContainer.Panel2.ResumeLayout(false);
            this._liveViewSplitContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _workloadsGridPanel;
        private WMSSummaryUserControl _wmsSummaryUserControl;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _liveViewSplitContainer;



    }
}
