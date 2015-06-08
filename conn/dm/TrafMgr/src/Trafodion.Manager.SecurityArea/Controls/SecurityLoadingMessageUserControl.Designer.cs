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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class SecurityLoadingMessageUserControl
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
            this._lookupPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.progressBar1 = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this._loadingMessageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._lookupPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _lookupPanel
            // 
            this._lookupPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._lookupPanel.Controls.Add(this.progressBar1);
            this._lookupPanel.Controls.Add(this._loadingMessageLabel);
            this._lookupPanel.Location = new System.Drawing.Point(16, 88);
            this._lookupPanel.Name = "_lookupPanel";
            this._lookupPanel.Size = new System.Drawing.Size(622, 156);
            this._lookupPanel.TabIndex = 5;
            // 
            // progressBar1
            // 
            this.progressBar1.Location = new System.Drawing.Point(104, 36);
            this.progressBar1.Name = "progressBar1";
            this.progressBar1.Size = new System.Drawing.Size(430, 18);
            this.progressBar1.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.progressBar1.TabIndex = 3;
            // 
            // _loadingMessageLabel
            // 
            this._loadingMessageLabel.AutoSize = true;
            this._loadingMessageLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._loadingMessageLabel.Location = new System.Drawing.Point(101, 14);
            this._loadingMessageLabel.Name = "_loadingMessageLabel";
            this._loadingMessageLabel.Size = new System.Drawing.Size(181, 13);
            this._loadingMessageLabel.TabIndex = 2;
            this._loadingMessageLabel.Text = "Loading Trafodion security policies ...";
            // 
            // SecurityLoadingMessageUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._lookupPanel);
            this.Name = "SecurityLoadingMessageUserControl";
            this.Size = new System.Drawing.Size(655, 333);
            this._lookupPanel.ResumeLayout(false);
            this._lookupPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _lookupPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar progressBar1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _loadingMessageLabel;
    }
}
