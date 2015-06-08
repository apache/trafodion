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
    partial class SQLOffenderCanvas
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SQLOffenderCanvas));
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._widgetCanvas = new Trafodion.Manager.Framework.WidgetCanvas();
            this.SuspendLayout();
            // 
            // toolTip1
            // 
            this.toolTip1.IsBalloon = true;
            // 
            // _widgetCanvas
            // 
            this._widgetCanvas.ActiveWidget = null;
            this._widgetCanvas.AllowDelete = true;
            this._widgetCanvas.AllowDrop = true;
            this._widgetCanvas.Dock = System.Windows.Forms.DockStyle.Fill;
            this._widgetCanvas.LayoutManager = null;
            this._widgetCanvas.Location = new System.Drawing.Point(0, 0);
            this._widgetCanvas.LockBackColor = System.Drawing.SystemColors.Control;
            this._widgetCanvas.Name = "_widgetCanvas";
            this._widgetCanvas.Size = new System.Drawing.Size(678, 345);
            this._widgetCanvas.TabIndex = 0;
            this._widgetCanvas.ThePersistenceKey = null;
            this._widgetCanvas.UnlockBackColor = System.Drawing.Color.Azure;
            this._widgetCanvas.ViewName = null;
            this._widgetCanvas.ViewNum = 0;
            this._widgetCanvas.ViewText = null;
            this._widgetCanvas.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("_widgetCanvas.WidgetsModel")));
            // 
            // SQLOffenderCanvas
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._widgetCanvas);
            this.Name = "SQLOffenderCanvas";
            this.Size = new System.Drawing.Size(678, 345);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
        private Framework.WidgetCanvas _widgetCanvas;
    }
}
