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
    partial class TabbedWorkloadUserControlWrapper
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
            this._theRootPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._theSystemOffenderTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theLiveViewTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theTriageTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanelUpperLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theRootPanel.SuspendLayout();
            this._theTabControl.SuspendLayout();
            this.theTopPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theRootPanel
            // 
            this._theRootPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theRootPanel.Controls.Add(this._theTabControl);
            this._theRootPanel.Controls.Add(this.theTopPanel);
            this._theRootPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theRootPanel.Location = new System.Drawing.Point(0, 0);
            this._theRootPanel.Name = "_theRootPanel";
            this._theRootPanel.Size = new System.Drawing.Size(744, 447);
            this._theRootPanel.TabIndex = 0;
            // 
            // _theTabControl
            // 
            this._theTabControl.Controls.Add(this._theSystemOffenderTabPage);
            this._theTabControl.Controls.Add(this._theLiveViewTab);
            this._theTabControl.Controls.Add(this._theTriageTab);
            this._theTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTabControl.HotTrack = true;
            this._theTabControl.Location = new System.Drawing.Point(0, 33);
            this._theTabControl.Multiline = true;
            this._theTabControl.Name = "_theTabControl";
            this._theTabControl.Padding = new System.Drawing.Point(10, 5);
            this._theTabControl.SelectedIndex = 0;
            this._theTabControl.Size = new System.Drawing.Size(744, 414);
            this._theTabControl.TabIndex = 0;
            // 
            // _theSystemOffenderTabPage
            // 
            this._theSystemOffenderTabPage.Location = new System.Drawing.Point(4, 26);
            this._theSystemOffenderTabPage.Name = "_theSystemOffenderTabPage";
            this._theSystemOffenderTabPage.Size = new System.Drawing.Size(736, 384);
            this._theSystemOffenderTabPage.TabIndex = 2;
            this._theSystemOffenderTabPage.Text = global::Trafodion.Manager.WorkloadArea.Properties.Resources.SystemOffender;
            this._theSystemOffenderTabPage.UseVisualStyleBackColor = true;
            // 
            // _theLiveViewTab
            // 
            this._theLiveViewTab.Location = new System.Drawing.Point(4, 26);
            this._theLiveViewTab.Name = "_theLiveViewTab";
            this._theLiveViewTab.Padding = new System.Windows.Forms.Padding(3);
            this._theLiveViewTab.Size = new System.Drawing.Size(736, 384);
            this._theLiveViewTab.TabIndex = 0;
            this._theLiveViewTab.Text = "Workload Monitor";
            this._theLiveViewTab.UseVisualStyleBackColor = true;
            // 
            // _theTriageTab
            // 
            this._theTriageTab.Location = new System.Drawing.Point(4, 26);
            this._theTriageTab.Name = "_theTriageTab";
            this._theTriageTab.Padding = new System.Windows.Forms.Padding(3);
            this._theTriageTab.Size = new System.Drawing.Size(736, 384);
            this._theTriageTab.TabIndex = 1;
            this._theTriageTab.Text = global::Trafodion.Manager.WorkloadArea.Properties.Resources.TriageSpace;
            this._theTriageTab.UseVisualStyleBackColor = true;
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.theTopPanel.Controls.Add(this.theTopPanelUpperLabel);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(744, 33);
            this.theTopPanel.TabIndex = 9;
            // 
            // theTopPanelUpperLabel
            // 
            this.theTopPanelUpperLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.theTopPanelUpperLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelUpperLabel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theTopPanelUpperLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelUpperLabel.Location = new System.Drawing.Point(5, 7);
            this.theTopPanelUpperLabel.Name = "theTopPanelUpperLabel";
            this.theTopPanelUpperLabel.ReadOnly = true;
            this.theTopPanelUpperLabel.Size = new System.Drawing.Size(732, 15);
            this.theTopPanelUpperLabel.TabIndex = 1;
            this.theTopPanelUpperLabel.Text = "Overall System Summary";
            // 
            // TabbedWorkloadUserControlWrapper
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theRootPanel);
            this.Name = "TabbedWorkloadUserControlWrapper";
            this.Size = new System.Drawing.Size(744, 447);
            this._theRootPanel.ResumeLayout(false);
            this._theTabControl.ResumeLayout(false);
            this.theTopPanel.ResumeLayout(false);
            this.theTopPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theRootPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _theTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theLiveViewTab;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theTriageTab;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theSystemOffenderTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theTopPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox theTopPanelUpperLabel;
    }
}
