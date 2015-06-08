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
    partial class UniversalWidgetCanvas
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UniversalWidgetCanvas));
            this._theUniversalWidgetCanvas = new Trafodion.Manager.Framework.WidgetCanvas();
            this.SuspendLayout();
            // 
            // _theUniversalWidgetCanvas
            // 
            this._theUniversalWidgetCanvas.ActiveWidget = null;
            this._theUniversalWidgetCanvas.AllowDelete = true;
            this._theUniversalWidgetCanvas.AllowDrop = true;
            this._theUniversalWidgetCanvas.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theUniversalWidgetCanvas.LayoutManager = null;
            this._theUniversalWidgetCanvas.Location = new System.Drawing.Point(0, 0);
            this._theUniversalWidgetCanvas.Name = "_theUniversalWidgetCanvas";
            this._theUniversalWidgetCanvas.Size = new System.Drawing.Size(707, 481);
            this._theUniversalWidgetCanvas.TabIndex = 0;
            this._theUniversalWidgetCanvas.ThePersistenceKey = null;
            this._theUniversalWidgetCanvas.ViewName = null;
            this._theUniversalWidgetCanvas.ViewNum = 0;
            this._theUniversalWidgetCanvas.ViewText = null;
            this._theUniversalWidgetCanvas.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("_theUniversalWidgetCanvas.WidgetsModel")));
            // 
            // UniversalWidgetCanvas
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theUniversalWidgetCanvas);
            this.Name = "UniversalWidgetCanvas";
            this.Size = new System.Drawing.Size(707, 481);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.WidgetCanvas _theUniversalWidgetCanvas;
    }
}
