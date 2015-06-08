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
﻿namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaDatasourceSummaryUserControl
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
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theBottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(786, 398);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // theBottomPanel
            // 
            this.theBottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theBottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.theBottomPanel.Location = new System.Drawing.Point(0, 398);
            this.theBottomPanel.Name = "theBottomPanel";
            this.theBottomPanel.Size = new System.Drawing.Size(786, 31);
            this.theBottomPanel.TabIndex = 1;
            // 
            // ConnectivityAreaDatasourceSummaryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this.theBottomPanel);
            this.Name = "ConnectivityAreaDatasourceSummaryUserControl";
            this.Size = new System.Drawing.Size(786, 429);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theBottomPanel;

    }
}
