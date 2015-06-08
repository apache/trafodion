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
    partial class WMSQueryPlan
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
            this._theCanvas = new Trafodion.Manager.UniversalWidget.Controls.UniversalWidgetCanvas();
            this.SuspendLayout();
            // 
            // _theCanvas
            // 
            this._theCanvas.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theCanvas.LayoutManager = null;
            this._theCanvas.Location = new System.Drawing.Point(0, 0);
            this._theCanvas.Name = "_theCanvas";
            this._theCanvas.Size = new System.Drawing.Size(771, 349);
            this._theCanvas.TabIndex = 0;
            // 
            // WMSQueryPlan
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theCanvas);
            this.Name = "WMSQueryPlan";
            this.Size = new System.Drawing.Size(771, 349);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.UniversalWidget.Controls.UniversalWidgetCanvas _theCanvas;
    }
}
