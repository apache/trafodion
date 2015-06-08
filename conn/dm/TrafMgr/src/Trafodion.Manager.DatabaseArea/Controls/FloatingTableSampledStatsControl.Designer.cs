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
    partial class FloatingTableSampledStatsControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FloatingTableSampledStatsControl));
            this.widgetCanvas = new Trafodion.Manager.Framework.WidgetCanvas();
            this.SuspendLayout();
            // 
            // widgetCanvas
            // 
            this.widgetCanvas.ActiveWidget = null;
            this.widgetCanvas.AllowDelete = true;
            this.widgetCanvas.AllowDrop = true;
            //this.widgetCanvas.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("widgetCanvas.BackgroundImage")));
            this.widgetCanvas.Dock = System.Windows.Forms.DockStyle.Fill;
            this.widgetCanvas.LayoutManager = null;
            this.widgetCanvas.Location = new System.Drawing.Point(0, 0);
            this.widgetCanvas.Margin = new System.Windows.Forms.Padding(4);
            this.widgetCanvas.Name = "widgetCanvas";
            this.widgetCanvas.Size = new System.Drawing.Size(800, 600);
            this.widgetCanvas.TabIndex = 0;
            this.widgetCanvas.ViewName = null;
            this.widgetCanvas.ViewNum = 0;
            this.widgetCanvas.ViewText = null;
            this.widgetCanvas.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("widgetCanvas.WidgetsModel")));
            // 
            // TableFloatingSampledStatsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.Controls.Add(this.widgetCanvas);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "TableFloatingSampledStatsControl";
            this.Size = new System.Drawing.Size(800, 600);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.WidgetCanvas widgetCanvas;
    }
}
