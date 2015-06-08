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
    partial class SnapshotMetricControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(OverallSummaryControl));
            this._theCanvas = new Trafodion.Manager.Framework.WidgetCanvas();
            this.SuspendLayout();
            // 
            // _theCanvas
            // 
            this._theCanvas.ActiveWidget = null;
            this._theCanvas.AllowDelete = true;
            this._theCanvas.AllowDrop = true;
            this._theCanvas.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theCanvas.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._theCanvas.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theCanvas.LayoutManager = null;
            this._theCanvas.Location = new System.Drawing.Point(0, 0);
            this._theCanvas.LockBackColor = System.Drawing.SystemColors.Control;
            this._theCanvas.Name = "_theCanvas";
            this._theCanvas.Size = new System.Drawing.Size(645, 350);
            this._theCanvas.TabIndex = 10;
            this._theCanvas.ThePersistenceKey = null;
            this._theCanvas.UnlockBackColor = System.Drawing.Color.Azure;
            this._theCanvas.ViewName = null;
            this._theCanvas.ViewNum = 0;
            this._theCanvas.ViewText = null;
            this._theCanvas.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("_theCanvas.WidgetsModel")));
            // 
            // OverallSummaryControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theCanvas);
            this.Name = "OverallSummaryControl";
            this.Size = new System.Drawing.Size(645, 350);
            this.Load += new System.EventHandler(this.SnapshotMetricControl_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.WidgetCanvas _theCanvas;

    }
}
