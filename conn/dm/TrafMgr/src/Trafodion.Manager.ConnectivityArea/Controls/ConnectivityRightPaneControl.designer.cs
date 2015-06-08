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
﻿using System;
using System.Windows.Forms;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityRightPaneControl
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
            this._connectivityTopTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._connectivityRightPanePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._connectivityRightPanePanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _connectivityTopTabControl
            // 
            this._connectivityTopTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._connectivityTopTabControl.Location = new System.Drawing.Point(0, 0);
            this._connectivityTopTabControl.Name = "_connectivityTopTabControl";
            this._connectivityTopTabControl.Padding = new System.Drawing.Point(10, 5);
            this._connectivityTopTabControl.SelectedIndex = 0;
            this._connectivityTopTabControl.Size = new System.Drawing.Size(385, 311);
            this._connectivityTopTabControl.TabIndex = 0;
            // 
            // _connectivityRightPanePanel
            // 
            this._connectivityRightPanePanel.Controls.Add(this._connectivityTopTabControl);
            this._connectivityRightPanePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._connectivityRightPanePanel.Location = new System.Drawing.Point(0, 0);
            this._connectivityRightPanePanel.Name = "_connectivityRightPanePanel";
            this._connectivityRightPanePanel.Size = new System.Drawing.Size(385, 311);
            this._connectivityRightPanePanel.TabIndex = 1;
            // 
            // ConnectivityRightPaneControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._connectivityRightPanePanel);
            this.Name = "ConnectivityRightPaneControl";
            this.Size = new System.Drawing.Size(385, 311);
            this._connectivityRightPanePanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _connectivityTopTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _connectivityRightPanePanel;
        //private ConnectivityAreaUserControl connectivityAreaUserControl;
    }
}
