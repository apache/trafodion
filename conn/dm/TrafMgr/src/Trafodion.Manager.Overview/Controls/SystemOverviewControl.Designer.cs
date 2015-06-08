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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class SystemOverviewControl
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
            this.components = new System.ComponentModel.Container();
            this.oneGuiTabControl1 = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.tabPage2 = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.systemMonitorControl = new Trafodion.Manager.OverviewArea.Controls.SystemMonitorControl();
            this.systemMessageTabPage = new Trafodion.Manager.OverviewArea.Controls.SystemMessageTabPage();
            this._systemMessageControl = new Trafodion.Manager.OverviewArea.Controls.SystemMessageControl();
            this.oneGuiTabControl1.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.systemMessageTabPage.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiTabControl1
            // 
            this.oneGuiTabControl1.Controls.Add(this.tabPage2);
            this.oneGuiTabControl1.Controls.Add(this.systemMessageTabPage);
            this.oneGuiTabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiTabControl1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiTabControl1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiTabControl1.Multiline = true;
            this.oneGuiTabControl1.Name = "oneGuiTabControl1";
            this.oneGuiTabControl1.Padding = new System.Drawing.Point(10, 5);
            this.oneGuiTabControl1.SelectedIndex = 0;
            this.oneGuiTabControl1.Size = new System.Drawing.Size(779, 453);
            this.oneGuiTabControl1.TabIndex = 0;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.systemMonitorControl);
            this.tabPage2.Location = new System.Drawing.Point(4, 26);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Size = new System.Drawing.Size(771, 423);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "System Monitor";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // systemMonitorControl
            // 
            this.systemMonitorControl.ConnectionDefn = null;
            this.systemMonitorControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.systemMonitorControl.Location = new System.Drawing.Point(0, 0);
            this.systemMonitorControl.Name = "systemMonitorControl";
            this.systemMonitorControl.Size = new System.Drawing.Size(771, 423);
            this.systemMonitorControl.TabIndex = 1;
            // 
            // systemMessageTabPage
            // 
            this.systemMessageTabPage.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(204)))), ((int)(((byte)(204)))), ((int)(((byte)(204)))));
            this.systemMessageTabPage.ConnectionDefinition = null;
            this.systemMessageTabPage.Controls.Add(this._systemMessageControl);
            this.systemMessageTabPage.Cursor = System.Windows.Forms.Cursors.Default;
            this.systemMessageTabPage.Location = new System.Drawing.Point(4, 26);
            this.systemMessageTabPage.Name = "systemMessageTabPage";
            this.systemMessageTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.systemMessageTabPage.Size = new System.Drawing.Size(771, 401);
            this.systemMessageTabPage.TabIndex = 0;
            this.systemMessageTabPage.Text = "System Message";
            this.systemMessageTabPage.UseVisualStyleBackColor = true;
            // 
            // _systemMessageControl
            // 
            this._systemMessageControl.ConnectionDefn = null;
            this._systemMessageControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._systemMessageControl.Location = new System.Drawing.Point(3, 3);
            this._systemMessageControl.Name = "_systemMessageControl";
            this._systemMessageControl.OverviewTreeView = null;
            this._systemMessageControl.Size = new System.Drawing.Size(765, 395);
            this._systemMessageControl.SystemMessage = null;
            this._systemMessageControl.TabIndex = 0;
            // 
            // SystemOverviewControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.oneGuiTabControl1);
            this.Name = "SystemOverviewControl";
            this.Size = new System.Drawing.Size(779, 453);
            this.oneGuiTabControl1.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.systemMessageTabPage.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionTabControl oneGuiTabControl1;
        private SystemMessageTabPage systemMessageTabPage;
        private SystemMessageControl _systemMessageControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage tabPage2;
        public SystemMonitorControl systemMonitorControl;
    }
}
