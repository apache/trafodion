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
    partial class SystemHealthStatesUserControl
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
            this._theTrafodionPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._thePanel = new System.Windows.Forms.FlowLayoutPanel();
            this._accessStatusLightUserControl = new Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl();
            this._databaseStatusLightUserControl = new Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl();
            this._foundationStatusLightUserControl = new Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl();
            this._osStatusLightUserControl = new Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl();
            this._serverStatusLightUserControl = new Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl();
            this._storageStatusLightUserControl = new Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl();
            this._theTrafodionPanel.SuspendLayout();
            this._thePanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theTrafodionPanel
            // 
            this._theTrafodionPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTrafodionPanel.Controls.Add(this._thePanel);
            this._theTrafodionPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTrafodionPanel.Location = new System.Drawing.Point(0, 0);
            this._theTrafodionPanel.Name = "_theTrafodionPanel";
            this._theTrafodionPanel.Size = new System.Drawing.Size(689, 122);
            this._theTrafodionPanel.TabIndex = 0;
            // 
            // _thePanel
            // 
            this._thePanel.AutoScroll = true;
            this._thePanel.AutoSize = true;
            this._thePanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._thePanel.Controls.Add(this._accessStatusLightUserControl);
            this._thePanel.Controls.Add(this._databaseStatusLightUserControl);
            this._thePanel.Controls.Add(this._foundationStatusLightUserControl);
            this._thePanel.Controls.Add(this._osStatusLightUserControl);
            this._thePanel.Controls.Add(this._serverStatusLightUserControl);
            this._thePanel.Controls.Add(this._storageStatusLightUserControl);
            this._thePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePanel.Location = new System.Drawing.Point(0, 0);
            this._thePanel.Name = "_thePanel";
            this._thePanel.Size = new System.Drawing.Size(689, 122);
            this._thePanel.TabIndex = 0;
            // 
            // _accessStatusLightUserControl
            // 
            this._accessStatusLightUserControl.AccessibleName = "Access";
            this._accessStatusLightUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._accessStatusLightUserControl.Key = "";
            this._accessStatusLightUserControl.Label = "Access";
            this._accessStatusLightUserControl.Location = new System.Drawing.Point(3, 3);
            this._accessStatusLightUserControl.Name = "_accessStatusLightUserControl";
            this._accessStatusLightUserControl.Size = new System.Drawing.Size(100, 107);
            this._accessStatusLightUserControl.State = -1;
            this._accessStatusLightUserControl.TabIndex = 0;
            this._accessStatusLightUserControl.ToolTipText = Properties.Resources.DetailsNotYetAvailable;
            // 
            // _databaseStatusLightUserControl
            // 
            this._databaseStatusLightUserControl.AccessibleName = "Database";
            this._databaseStatusLightUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._databaseStatusLightUserControl.Key = "";
            this._databaseStatusLightUserControl.Label = "Database";
            this._databaseStatusLightUserControl.Location = new System.Drawing.Point(109, 3);
            this._databaseStatusLightUserControl.Name = "_databaseStatusLightUserControl";
            this._databaseStatusLightUserControl.Size = new System.Drawing.Size(100, 107);
            this._databaseStatusLightUserControl.State = -1;
            this._databaseStatusLightUserControl.TabIndex = 1;
            this._databaseStatusLightUserControl.ToolTipText = Properties.Resources.DetailsNotYetAvailable;
            // 
            // _foundationStatusLightUserControl
            // 
            this._foundationStatusLightUserControl.AccessibleName = "Foundation";
            this._foundationStatusLightUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._foundationStatusLightUserControl.Key = "";
            this._foundationStatusLightUserControl.Label = "Foundation";
            this._foundationStatusLightUserControl.Location = new System.Drawing.Point(215, 3);
            this._foundationStatusLightUserControl.Name = "_foundationStatusLightUserControl";
            this._foundationStatusLightUserControl.Size = new System.Drawing.Size(100, 107);
            this._foundationStatusLightUserControl.State = -1;
            this._foundationStatusLightUserControl.TabIndex = 2;
            this._foundationStatusLightUserControl.ToolTipText = Properties.Resources.DetailsNotYetAvailable;
            // 
            // _osStatusLightUserControl
            // 
            this._osStatusLightUserControl.AccessibleName = "OS";
            this._osStatusLightUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._osStatusLightUserControl.Key = "";
            this._osStatusLightUserControl.Label = "OS";
            this._osStatusLightUserControl.Location = new System.Drawing.Point(321, 3);
            this._osStatusLightUserControl.Name = "_osStatusLightUserControl";
            this._osStatusLightUserControl.Size = new System.Drawing.Size(100, 107);
            this._osStatusLightUserControl.State = -1;
            this._osStatusLightUserControl.TabIndex = 3;
            this._osStatusLightUserControl.ToolTipText = Properties.Resources.DetailsNotYetAvailable;
            // 
            // _serverStatusLightUserControl
            // 
            this._serverStatusLightUserControl.AccessibleName = "Server";
            this._serverStatusLightUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._serverStatusLightUserControl.Key = "";
            this._serverStatusLightUserControl.Label = "Server";
            this._serverStatusLightUserControl.Location = new System.Drawing.Point(427, 3);
            this._serverStatusLightUserControl.Name = "_serverStatusLightUserControl";
            this._serverStatusLightUserControl.Size = new System.Drawing.Size(100, 107);
            this._serverStatusLightUserControl.State = -1;
            this._serverStatusLightUserControl.TabIndex = 4;
            this._serverStatusLightUserControl.ToolTipText = Properties.Resources.DetailsNotYetAvailable;
            // 
            // _storageStatusLightUserControl
            // 
            this._storageStatusLightUserControl.AccessibleName = "Storage";
            this._storageStatusLightUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._storageStatusLightUserControl.Key = "";
            this._storageStatusLightUserControl.Label = "Storage";
            this._storageStatusLightUserControl.Location = new System.Drawing.Point(533, 3);
            this._storageStatusLightUserControl.Name = "_storageStatusLightUserControl";
            this._storageStatusLightUserControl.Size = new System.Drawing.Size(100, 107);
            this._storageStatusLightUserControl.State = -1;
            this._storageStatusLightUserControl.TabIndex = 5;
            this._storageStatusLightUserControl.ToolTipText = Properties.Resources.DetailsNotYetAvailable;
            // 
            // SystemHealthStatesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theTrafodionPanel);
            this.Name = "SystemHealthStatesUserControl";
            this.Size = new System.Drawing.Size(689, 122);
            this._theTrafodionPanel.ResumeLayout(false);
            this._theTrafodionPanel.PerformLayout();
            this._thePanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionPanel _theTrafodionPanel;
        private System.Windows.Forms.FlowLayoutPanel _thePanel;
        private Framework.Controls.TrafodionStatusLightUserControl _accessStatusLightUserControl;
        private Framework.Controls.TrafodionStatusLightUserControl _databaseStatusLightUserControl;
        private Framework.Controls.TrafodionStatusLightUserControl _foundationStatusLightUserControl;
        private Framework.Controls.TrafodionStatusLightUserControl _osStatusLightUserControl;
        private Framework.Controls.TrafodionStatusLightUserControl _serverStatusLightUserControl;
        private Framework.Controls.TrafodionStatusLightUserControl _storageStatusLightUserControl;
    }
}
