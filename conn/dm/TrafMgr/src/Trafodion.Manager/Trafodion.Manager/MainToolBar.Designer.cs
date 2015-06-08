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
﻿namespace Trafodion.Manager.Framework
{
    partial class MainToolBar
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainToolBar));
            this.theMainToolBarStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.connectToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.disconnectToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.systemToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.sytemToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
            this.openToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.saveToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.openSaveToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
            this.cutToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.copyToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.pasteToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.cutCopyPasteToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
            this.sqlWhiteBoardToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.mmToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.nciToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.optionsToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.eventViewerToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.auditLogToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.downloadOSIMToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.updateConfigurationToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._runScriptToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolsToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
            this.windowManagerToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.windowManagerToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
            this.parentToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.previousToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.nextToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.arrowsToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
            this.helpToolStripButton = new System.Windows.Forms.ToolStripButton();
            this.theMainToolBarStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // theMainToolBarStrip
            // 
            this.theMainToolBarStrip.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theMainToolBarStrip.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.theMainToolBarStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.connectToolStripButton,
            this.disconnectToolStripButton,
            this.systemToolStripButton,
            this.sytemToolStripSeparator,
            this.openToolStripButton,
            this.saveToolStripButton,
            this.openSaveToolStripSeparator,
            this.cutToolStripButton,
            this.copyToolStripButton,
            this.pasteToolStripButton,
            this.cutCopyPasteToolStripSeparator,
            this.sqlWhiteBoardToolStripButton,
            this.mmToolStripButton,
            this.nciToolStripButton,
            this.optionsToolStripButton,
            this.eventViewerToolStripButton,
            this.auditLogToolStripButton,
            this.downloadOSIMToolStripButton,
            this.updateConfigurationToolStripButton,
            this._runScriptToolStripButton,
            this.toolsToolStripSeparator,
            this.windowManagerToolStripButton,
            this.windowManagerToolStripSeparator,
            this.parentToolStripButton,
            this.previousToolStripButton,
            this.nextToolStripButton,
            this.arrowsToolStripSeparator,
            this.helpToolStripButton});
            this.theMainToolBarStrip.Location = new System.Drawing.Point(0, 0);
            this.theMainToolBarStrip.Name = "theMainToolBarStrip";
            this.theMainToolBarStrip.Size = new System.Drawing.Size(684, 25);
            this.theMainToolBarStrip.TabIndex = 2;
            this.theMainToolBarStrip.Text = "theMainToolBarStrip";
            // 
            // connectToolStripButton
            // 
            this.connectToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.connectToolStripButton.Enabled = false;
            this.connectToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.ConnectIcon;
            this.connectToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.connectToolStripButton.Name = "connectToolStripButton";
            this.connectToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.connectToolStripButton.Text = "Connect / Edit System";
            // 
            // disconnectToolStripButton
            // 
            this.disconnectToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.disconnectToolStripButton.Enabled = false;
            this.disconnectToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.DisconnectIcon;
            this.disconnectToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.disconnectToolStripButton.Name = "disconnectToolStripButton";
            this.disconnectToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.disconnectToolStripButton.Text = "Disconnect";
            // 
            // systemToolStripButton
            // 
            this.systemToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.systemToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.MultipleServersIcon;
            this.systemToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.systemToolStripButton.Name = "systemToolStripButton";
            this.systemToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.systemToolStripButton.Text = "Systems Tool";
            // 
            // sytemToolStripSeparator
            // 
            this.sytemToolStripSeparator.Name = "sytemToolStripSeparator";
            this.sytemToolStripSeparator.Size = new System.Drawing.Size(6, 25);
            // 
            // openToolStripButton
            // 
            this.openToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.openToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("openToolStripButton.Image")));
            this.openToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.openToolStripButton.Name = "openToolStripButton";
            this.openToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.openToolStripButton.Text = "&Open";
            this.openToolStripButton.ToolTipText = "Import Persistence";
            // 
            // saveToolStripButton
            // 
            this.saveToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.saveToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("saveToolStripButton.Image")));
            this.saveToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.saveToolStripButton.Name = "saveToolStripButton";
            this.saveToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.saveToolStripButton.Text = "&Save";
            this.saveToolStripButton.ToolTipText = "Save Persistence";
            // 
            // openSaveToolStripSeparator
            // 
            this.openSaveToolStripSeparator.Name = "openSaveToolStripSeparator";
            this.openSaveToolStripSeparator.Size = new System.Drawing.Size(6, 25);
            // 
            // cutToolStripButton
            // 
            this.cutToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.cutToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("cutToolStripButton.Image")));
            this.cutToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.cutToolStripButton.Name = "cutToolStripButton";
            this.cutToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.cutToolStripButton.Text = "C&ut";
            // 
            // copyToolStripButton
            // 
            this.copyToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.copyToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("copyToolStripButton.Image")));
            this.copyToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.copyToolStripButton.Name = "copyToolStripButton";
            this.copyToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.copyToolStripButton.Text = "&Copy";
            // 
            // pasteToolStripButton
            // 
            this.pasteToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.pasteToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("pasteToolStripButton.Image")));
            this.pasteToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.pasteToolStripButton.Name = "pasteToolStripButton";
            this.pasteToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.pasteToolStripButton.Text = "&Paste";
            // 
            // cutCopyPasteToolStripSeparator
            // 
            this.cutCopyPasteToolStripSeparator.Name = "cutCopyPasteToolStripSeparator";
            this.cutCopyPasteToolStripSeparator.Size = new System.Drawing.Size(6, 25);
            // 
            // sqlWhiteBoardToolStripButton
            // 
            this.sqlWhiteBoardToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.sqlWhiteBoardToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.WhiteBoardIcon;
            this.sqlWhiteBoardToolStripButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            this.sqlWhiteBoardToolStripButton.Name = "sqlWhiteBoardToolStripButton";
            this.sqlWhiteBoardToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.sqlWhiteBoardToolStripButton.Text = "SQL Whiteboard";
            this.sqlWhiteBoardToolStripButton.ToolTipText = "SQL Whiteboard";
            // 
            // mmToolStripButton
            // 
            this.mmToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.mmToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.MetricMinerIcon;
            this.mmToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.mmToolStripButton.Name = "mmToolStripButton";
            this.mmToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.mmToolStripButton.Text = "Metric Miner";
            // 
            // nciToolStripButton
            // 
            this.nciToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.nciToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.CommandWindowIcon;
            this.nciToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.nciToolStripButton.Name = "nciToolStripButton";
            this.nciToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.nciToolStripButton.Text = global::Trafodion.Manager.Properties.Resources.NCI;
            this.nciToolStripButton.ToolTipText = global::Trafodion.Manager.Properties.Resources.NCI;
            // 
            // optionsToolStripButton
            // 
            this.optionsToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.optionsToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.PropertiesIcon;
            this.optionsToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.optionsToolStripButton.Name = "optionsToolStripButton";
            this.optionsToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.optionsToolStripButton.Text = "Options";
            this.optionsToolStripButton.ToolTipText = "Options";
            // 
            // eventViewerToolStripButton
            // 
            this.eventViewerToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.eventViewerToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.EventViewerIcon;
            this.eventViewerToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.eventViewerToolStripButton.Name = "eventViewerToolStripButton";
            this.eventViewerToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.eventViewerToolStripButton.Text = "Event Viewer";
            this.eventViewerToolStripButton.Visible = false;
            // 
            // auditLogToolStripButton
            // 
            this.auditLogToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.auditLogToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.auditlog;
            this.auditLogToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.auditLogToolStripButton.Name = "auditLogToolStripButton";
            this.auditLogToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.auditLogToolStripButton.Text = "Security Audit Log Viewer";
            this.auditLogToolStripButton.Visible = false;
            // 
            // downloadOSIMToolStripButton
            // 
            this.downloadOSIMToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.downloadOSIMToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.download;
            this.downloadOSIMToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.downloadOSIMToolStripButton.Name = "downloadOSIMToolStripButton";
            this.downloadOSIMToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.downloadOSIMToolStripButton.Text = "Download OSIM Data Files";
            this.downloadOSIMToolStripButton.Visible = false;
            // 
            // updateConfigurationToolStripButton
            // 
            this.updateConfigurationToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.updateConfigurationToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.config;
            this.updateConfigurationToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.updateConfigurationToolStripButton.Name = "updateConfigurationToolStripButton";
            this.updateConfigurationToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.updateConfigurationToolStripButton.Text = "Update LDAP Configuration";
            this.updateConfigurationToolStripButton.Visible = false;
            this.updateConfigurationToolStripButton.EnabledChanged += new System.EventHandler(this.updateConfigurationToolStripButton_EnabledChanged);
            // 
            // _runScriptToolStripButton
            // 
            this._runScriptToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._runScriptToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.RunIcon;
            this._runScriptToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._runScriptToolStripButton.Name = "_runScriptToolStripButton";
            this._runScriptToolStripButton.Size = new System.Drawing.Size(23, 22);
            this._runScriptToolStripButton.ToolTipText = "Run Script";
            this._runScriptToolStripButton.Visible = false;
            // 
            // toolsToolStripSeparator
            // 
            this.toolsToolStripSeparator.Name = "toolsToolStripSeparator";
            this.toolsToolStripSeparator.Size = new System.Drawing.Size(6, 25);
            // 
            // windowManagerToolStripButton
            // 
            this.windowManagerToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.windowManagerToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.WindowsManagerIcon;
            this.windowManagerToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.windowManagerToolStripButton.Name = "windowManagerToolStripButton";
            this.windowManagerToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.windowManagerToolStripButton.Text = "Windows Manager";
            // 
            // windowManagerToolStripSeparator
            // 
            this.windowManagerToolStripSeparator.Name = "windowManagerToolStripSeparator";
            this.windowManagerToolStripSeparator.Size = new System.Drawing.Size(6, 25);
            // 
            // parentToolStripButton
            // 
            this.parentToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.parentToolStripButton.Enabled = false;
            this.parentToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.UpArrowIcon;
            this.parentToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.parentToolStripButton.Name = "parentToolStripButton";
            this.parentToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.parentToolStripButton.ToolTipText = "Go to Parent";
            // 
            // previousToolStripButton
            // 
            this.previousToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.previousToolStripButton.Enabled = false;
            this.previousToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.LeftArrowIcon;
            this.previousToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.previousToolStripButton.Name = "previousToolStripButton";
            this.previousToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.previousToolStripButton.Text = "toolStripButton1";
            this.previousToolStripButton.ToolTipText = "Go to Previous";
            // 
            // nextToolStripButton
            // 
            this.nextToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.nextToolStripButton.Enabled = false;
            this.nextToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.RightArrowIcon;
            this.nextToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.nextToolStripButton.Name = "nextToolStripButton";
            this.nextToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.nextToolStripButton.ToolTipText = "Go to Next";
            // 
            // arrowsToolStripSeparator
            // 
            this.arrowsToolStripSeparator.Name = "arrowsToolStripSeparator";
            this.arrowsToolStripSeparator.Size = new System.Drawing.Size(6, 25);
            // 
            // helpToolStripButton
            // 
            this.helpToolStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.helpToolStripButton.Image = ((System.Drawing.Image)(resources.GetObject("helpToolStripButton.Image")));
            this.helpToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.helpToolStripButton.Name = "helpToolStripButton";
            this.helpToolStripButton.Size = new System.Drawing.Size(23, 22);
            this.helpToolStripButton.Text = "He&lp";
            // 
            // MainToolBar
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.theMainToolBarStrip);
            this.Name = "MainToolBar";
            this.Size = new System.Drawing.Size(684, 25);
            this.Load += new System.EventHandler(this.MainToolBar_Load);
            this.theMainToolBarStrip.ResumeLayout(false);
            this.theMainToolBarStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip theMainToolBarStrip;
        private System.Windows.Forms.ToolStripButton openToolStripButton;
        private System.Windows.Forms.ToolStripButton saveToolStripButton;
        private System.Windows.Forms.ToolStripSeparator openSaveToolStripSeparator;
        private System.Windows.Forms.ToolStripButton cutToolStripButton;
        private System.Windows.Forms.ToolStripButton copyToolStripButton;
        private System.Windows.Forms.ToolStripButton pasteToolStripButton;
        private System.Windows.Forms.ToolStripSeparator cutCopyPasteToolStripSeparator;
        private System.Windows.Forms.ToolStripButton sqlWhiteBoardToolStripButton;
        private System.Windows.Forms.ToolStripButton nciToolStripButton;
        private System.Windows.Forms.ToolStripButton optionsToolStripButton;
        private System.Windows.Forms.ToolStripSeparator toolsToolStripSeparator;
        private System.Windows.Forms.ToolStripButton helpToolStripButton;
        private System.Windows.Forms.ToolStripButton windowManagerToolStripButton;
        private System.Windows.Forms.ToolStripSeparator windowManagerToolStripSeparator;
        private System.Windows.Forms.ToolStripButton systemToolStripButton;
        private System.Windows.Forms.ToolStripButton connectToolStripButton;
        private System.Windows.Forms.ToolStripButton disconnectToolStripButton;
        private System.Windows.Forms.ToolStripSeparator sytemToolStripSeparator;
        private System.Windows.Forms.ToolStripButton parentToolStripButton;
        private System.Windows.Forms.ToolStripButton previousToolStripButton;
        private System.Windows.Forms.ToolStripButton nextToolStripButton;
        private System.Windows.Forms.ToolStripSeparator arrowsToolStripSeparator;
        private System.Windows.Forms.ToolStripButton mmToolStripButton;
        private System.Windows.Forms.ToolStripButton eventViewerToolStripButton;
        private System.Windows.Forms.ToolStripButton auditLogToolStripButton;
        private System.Windows.Forms.ToolStripButton downloadOSIMToolStripButton;
        private System.Windows.Forms.ToolStripButton updateConfigurationToolStripButton;
        private System.Windows.Forms.ToolStripButton _runScriptToolStripButton;
    }
}
