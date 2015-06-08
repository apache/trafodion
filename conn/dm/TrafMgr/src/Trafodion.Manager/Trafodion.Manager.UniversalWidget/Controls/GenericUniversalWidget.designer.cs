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
    partial class GenericUniversalWidget
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(GenericUniversalWidget));
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionSplitContainer1 = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.TrafodionSplitContainer2 = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theDisplayPanel = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theConfigurationContainer = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theLowerTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._theOutputTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theParameterTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theDocumentationTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theErrorTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.statusStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionStatusStrip();
            this._theRefreshLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._theStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._theStatusProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this._theCancelQueryLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._theToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theConnectionLabel = new System.Windows.Forms.ToolStripLabel();
            this._theConnections = new System.Windows.Forms.ToolStripComboBox();
            this._theCatalogLabel = new System.Windows.Forms.ToolStripLabel();
            this._theCatalogs = new System.Windows.Forms.ToolStripComboBox();
            this._theSchemaLabel = new System.Windows.Forms.ToolStripLabel();
            this._theSchemas = new System.Windows.Forms.ToolStripComboBox();
            this._theSplitter1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator();
            this._theStopQuery = new System.Windows.Forms.ToolStripButton();
            this._theRefreshButton = new System.Windows.Forms.ToolStripButton();
            this._theRefreshTimerButton = new System.Windows.Forms.ToolStripButton();
            this._theTimerSetupButton = new System.Windows.Forms.ToolStripDropDownButton();
            this.Sec30MenuItem = new Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem();
            this.toolStripSeparator1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator();
            this.Minute1MenuItem = new Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem();
            this.Minute2MenuItem = new Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem();
            this.Minute5MenuItem = new Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem();
            this.Minute10MenuItem = new Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem();
            this.Minute15MenuItem = new Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem();
            this.Minute30MenuItem = new Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem();
            this._theExportButtonSeparator = new Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator();
            this._theExportButton = new System.Windows.Forms.ToolStripDropDownButton();
            this._theGraphDetailsSplitter = new Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator();
            this._theShowGraphButton = new System.Windows.Forms.ToolStripDropDownButton();
            this._theShowDetails = new System.Windows.Forms.ToolStripButton();
            this._thePropertiesSplitter = new Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator();
            this._theShowProperties = new System.Windows.Forms.ToolStripButton();
            this._theHelpButton = new System.Windows.Forms.ToolStripButton();
            this._theRowCount = new System.Windows.Forms.ToolStripTextBox();
            this._theRowCountLabel = new System.Windows.Forms.ToolStripLabel();
            this._theCustomToolStripButtonSeparator = new Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator();
            this._theWidgetCanvas = new Trafodion.Manager.Framework.WidgetCanvas();
            this.TrafodionPanel1.SuspendLayout();
            this.TrafodionSplitContainer1.Panel1.SuspendLayout();
            this.TrafodionSplitContainer1.Panel2.SuspendLayout();
            this.TrafodionSplitContainer1.SuspendLayout();
            this.TrafodionSplitContainer2.Panel1.SuspendLayout();
            this.TrafodionSplitContainer2.Panel2.SuspendLayout();
            this.TrafodionSplitContainer2.SuspendLayout();
            this._theDisplayPanel.SuspendLayout();
            this._theLowerTabControl.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this._theToolStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.TrafodionSplitContainer1);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 27);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(838, 558);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // TrafodionSplitContainer1
            // 
            this.TrafodionSplitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionSplitContainer1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionSplitContainer1.Name = "TrafodionSplitContainer1";
            this.TrafodionSplitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // TrafodionSplitContainer1.Panel1
            // 
            this.TrafodionSplitContainer1.Panel1.Controls.Add(this.TrafodionSplitContainer2);
            // 
            // TrafodionSplitContainer1.Panel2
            // 
            this.TrafodionSplitContainer1.Panel2.Controls.Add(this._theLowerTabControl);
            this.TrafodionSplitContainer1.Size = new System.Drawing.Size(838, 558);
            this.TrafodionSplitContainer1.SplitterDistance = 393;
            this.TrafodionSplitContainer1.SplitterWidth = 9;
            this.TrafodionSplitContainer1.TabIndex = 0;
            // 
            // TrafodionSplitContainer2
            // 
            this.TrafodionSplitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionSplitContainer2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionSplitContainer2.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.TrafodionSplitContainer2.Name = "TrafodionSplitContainer2";
            // 
            // TrafodionSplitContainer2.Panel1
            // 
            this.TrafodionSplitContainer2.Panel1.Controls.Add(this._theDisplayPanel);
            // 
            // TrafodionSplitContainer2.Panel2
            // 
            this.TrafodionSplitContainer2.Panel2.Controls.Add(this._theConfigurationContainer);
            this.TrafodionSplitContainer2.Size = new System.Drawing.Size(838, 393);
            this.TrafodionSplitContainer2.SplitterDistance = 622;
            this.TrafodionSplitContainer2.SplitterWidth = 9;
            this.TrafodionSplitContainer2.TabIndex = 0;
            // 
            // _theDisplayPanel
            // 
            this._theDisplayPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theDisplayPanel.Location = new System.Drawing.Point(0, 0);
            this._theDisplayPanel.Name = "_theDisplayPanel";
            this._theDisplayPanel.Size = new System.Drawing.Size(622, 393);
            this._theDisplayPanel.SplitterDistance = 391;
            this._theDisplayPanel.SplitterWidth = 9;
            this._theDisplayPanel.TabIndex = 0;
            // 
            // _theConfigurationContainer
            // 
            this._theConfigurationContainer.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theConfigurationContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theConfigurationContainer.Location = new System.Drawing.Point(0, 0);
            this._theConfigurationContainer.Name = "_theConfigurationContainer";
            this._theConfigurationContainer.Size = new System.Drawing.Size(207, 393);
            this._theConfigurationContainer.TabIndex = 0;
            // 
            // _theLowerTabControl
            // 
            this._theLowerTabControl.Controls.Add(this._theOutputTab);
            this._theLowerTabControl.Controls.Add(this._theParameterTabPage);
            this._theLowerTabControl.Controls.Add(this._theDocumentationTabPage);
            this._theLowerTabControl.Controls.Add(this._theErrorTabPage);
            this._theLowerTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theLowerTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLowerTabControl.HotTrack = true;
            this._theLowerTabControl.Location = new System.Drawing.Point(0, 0);
            this._theLowerTabControl.Multiline = true;
            this._theLowerTabControl.Name = "_theLowerTabControl";
            this._theLowerTabControl.Padding = new System.Drawing.Point(10, 5);
            this._theLowerTabControl.SelectedIndex = 0;
            this._theLowerTabControl.Size = new System.Drawing.Size(838, 156);
            this._theLowerTabControl.TabIndex = 0;
            // 
            // _theOutputTab
            // 
            this._theOutputTab.Location = new System.Drawing.Point(4, 26);
            this._theOutputTab.Name = "_theOutputTab";
            this._theOutputTab.Size = new System.Drawing.Size(830, 126);
            this._theOutputTab.TabIndex = 3;
            this._theOutputTab.Text = "Console";
            this._theOutputTab.UseVisualStyleBackColor = true;
            // 
            // _theParameterTabPage
            // 
            this._theParameterTabPage.Location = new System.Drawing.Point(4, 26);
            this._theParameterTabPage.Name = "_theParameterTabPage";
            this._theParameterTabPage.Padding = new System.Windows.Forms.Padding(3);
            this._theParameterTabPage.Size = new System.Drawing.Size(830, 126);
            this._theParameterTabPage.TabIndex = 0;
            this._theParameterTabPage.Text = "Parameters";
            this._theParameterTabPage.UseVisualStyleBackColor = true;
            // 
            // _theDocumentationTabPage
            // 
            this._theDocumentationTabPage.Location = new System.Drawing.Point(4, 26);
            this._theDocumentationTabPage.Name = "_theDocumentationTabPage";
            this._theDocumentationTabPage.Padding = new System.Windows.Forms.Padding(3);
            this._theDocumentationTabPage.Size = new System.Drawing.Size(830, 126);
            this._theDocumentationTabPage.TabIndex = 1;
            this._theDocumentationTabPage.Text = "Documentation";
            this._theDocumentationTabPage.UseVisualStyleBackColor = true;
            // 
            // _theErrorTabPage
            // 
            this._theErrorTabPage.Location = new System.Drawing.Point(4, 26);
            this._theErrorTabPage.Name = "_theErrorTabPage";
            this._theErrorTabPage.Size = new System.Drawing.Size(830, 126);
            this._theErrorTabPage.TabIndex = 2;
            this._theErrorTabPage.Text = "Errors";
            this._theErrorTabPage.UseVisualStyleBackColor = true;
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theRefreshLabel,
            this._theStatusLabel,
            this._theStatusProgressBar,
            this._theCancelQueryLabel});
            this.statusStrip1.Location = new System.Drawing.Point(0, 585);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(838, 22);
            this.statusStrip1.SizingGrip = false;
            this.statusStrip1.TabIndex = 2;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // _theRefreshLabel
            // 
            this._theRefreshLabel.AutoSize = false;
            this._theRefreshLabel.BackColor = System.Drawing.SystemColors.Control;
            this._theRefreshLabel.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
            this._theRefreshLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this._theRefreshLabel.Margin = new System.Windows.Forms.Padding(0, 3, 4, 2);
            this._theRefreshLabel.Name = "_theRefreshLabel";
            this._theRefreshLabel.Size = new System.Drawing.Size(120, 17);
            this._theRefreshLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theStatusLabel
            // 
            this._theStatusLabel.AutoSize = false;
            this._theStatusLabel.BackColor = System.Drawing.SystemColors.Control;
            this._theStatusLabel.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
            this._theStatusLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this._theStatusLabel.Margin = new System.Windows.Forms.Padding(0, 3, 4, 2);
            this._theStatusLabel.Name = "_theStatusLabel";
            this._theStatusLabel.Size = new System.Drawing.Size(120, 17);
            this._theStatusLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._theStatusLabel.Click += new System.EventHandler(this._theStatusLabel_Click);
            // 
            // _theStatusProgressBar
            // 
            this._theStatusProgressBar.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theStatusProgressBar.AutoSize = false;
            this._theStatusProgressBar.Margin = new System.Windows.Forms.Padding(1, 3, 4, 3);
            this._theStatusProgressBar.Name = "_theStatusProgressBar";
            this._theStatusProgressBar.Size = new System.Drawing.Size(100, 16);
            this._theStatusProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this._theStatusProgressBar.Visible = false;
            // 
            // _theCancelQueryLabel
            // 
            this._theCancelQueryLabel.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theCancelQueryLabel.Enabled = false;
            this._theCancelQueryLabel.Image = ((System.Drawing.Image)(resources.GetObject("_theCancelQueryLabel.Image")));
            this._theCancelQueryLabel.Name = "_theCancelQueryLabel";
            this._theCancelQueryLabel.Size = new System.Drawing.Size(16, 17);
            this._theCancelQueryLabel.Visible = false;
            this._theCancelQueryLabel.Click += new System.EventHandler(this._theCancelQuery_Click);
            // 
            // _theToolStrip
            // 
            this._theToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theConnectionLabel,
            this._theConnections,
            this._theCatalogLabel,
            this._theCatalogs,
            this._theSchemaLabel,
            this._theSchemas,
            this._theSplitter1,
            this._theStopQuery,
            this._theRefreshButton,
            this._theRefreshTimerButton,
            this._theTimerSetupButton,
            this._theExportButtonSeparator,
            this._theExportButton,
            this._theGraphDetailsSplitter,
            this._theShowGraphButton,
            this._theShowDetails,
            this._thePropertiesSplitter,
            this._theShowProperties,
            this._theHelpButton,
            this._theRowCount,
            this._theRowCountLabel,
            this._theCustomToolStripButtonSeparator});
            this._theToolStrip.Location = new System.Drawing.Point(0, 0);
            this._theToolStrip.Name = "_theToolStrip";
            this._theToolStrip.Size = new System.Drawing.Size(838, 27);
            this._theToolStrip.TabIndex = 3;
            this._theToolStrip.Text = "toolStrip1";
            // 
            // _theConnectionLabel
            // 
            this._theConnectionLabel.Name = "_theConnectionLabel";
            this._theConnectionLabel.Size = new System.Drawing.Size(75, 24);
            this._theConnectionLabel.Text = "Connection :";
            // 
            // _theConnections
            // 
            this._theConnections.Name = "_theConnections";
            this._theConnections.Size = new System.Drawing.Size(121, 27);
            this._theConnections.ToolTipText = "All connected connections";
            // 
            // _theCatalogLabel
            // 
            this._theCatalogLabel.Name = "_theCatalogLabel";
            this._theCatalogLabel.Size = new System.Drawing.Size(54, 24);
            this._theCatalogLabel.Text = "Catalog :";
            // 
            // _theCatalogs
            // 
            this._theCatalogs.Name = "_theCatalogs";
            this._theCatalogs.Size = new System.Drawing.Size(121, 27);
            this._theCatalogs.ToolTipText = "Catalogs for the selected connection";
            // 
            // _theSchemaLabel
            // 
            this._theSchemaLabel.Name = "_theSchemaLabel";
            this._theSchemaLabel.Size = new System.Drawing.Size(55, 24);
            this._theSchemaLabel.Text = "Schema :";
            // 
            // _theSchemas
            // 
            this._theSchemas.Name = "_theSchemas";
            this._theSchemas.Size = new System.Drawing.Size(121, 27);
            this._theSchemas.ToolTipText = "Schemas for the selected catalog";
            // 
            // _theSplitter1
            // 
            this._theSplitter1.IsFrameworkMenu = false;
            this._theSplitter1.Name = "_theSplitter1";
            this._theSplitter1.Size = new System.Drawing.Size(6, 27);
            // 
            // _theStopQuery
            // 
            this._theStopQuery.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theStopQuery.Enabled = false;
            this._theStopQuery.Image = ((System.Drawing.Image)(resources.GetObject("_theStopQuery.Image")));
            this._theStopQuery.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theStopQuery.Name = "_theStopQuery";
            this._theStopQuery.Size = new System.Drawing.Size(23, 24);
            this._theStopQuery.Text = "Stop data provider";
            this._theStopQuery.ToolTipText = "Stop data provider";
            this._theStopQuery.Click += new System.EventHandler(this._theStopQuery_Click);
            // 
            // _theRefreshButton
            // 
            this._theRefreshButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRefreshButton.Image")));
            this._theRefreshButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRefreshButton.Name = "_theRefreshButton";
            this._theRefreshButton.Size = new System.Drawing.Size(23, 24);
            this._theRefreshButton.Text = "Refresh data";
            this._theRefreshButton.Click += new System.EventHandler(this._theRefreshButton_Click);
            // 
            // _theRefreshTimerButton
            // 
            this._theRefreshTimerButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRefreshTimerButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRefreshTimerButton.Image")));
            this._theRefreshTimerButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRefreshTimerButton.Name = "_theRefreshTimerButton";
            this._theRefreshTimerButton.Size = new System.Drawing.Size(23, 24);
            this._theRefreshTimerButton.Text = "Resume timer";
            this._theRefreshTimerButton.Click += new System.EventHandler(this._theRefreshTimerButton_Click);
            // 
            // _theTimerSetupButton
            // 
            this._theTimerSetupButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theTimerSetupButton.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.Sec30MenuItem,
            this.toolStripSeparator1,
            this.Minute1MenuItem,
            this.Minute2MenuItem,
            this.Minute5MenuItem,
            this.Minute10MenuItem,
            this.Minute15MenuItem,
            this.Minute30MenuItem});
            this._theTimerSetupButton.Image = ((System.Drawing.Image)(resources.GetObject("_theTimerSetupButton.Image")));
            this._theTimerSetupButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theTimerSetupButton.Name = "_theTimerSetupButton";
            this._theTimerSetupButton.Size = new System.Drawing.Size(29, 24);
            this._theTimerSetupButton.Text = "Set refresh time";
            // 
            // Sec30MenuItem
            // 
            this.Sec30MenuItem.IsFrameworkMenu = false;
            this.Sec30MenuItem.Name = "Sec30MenuItem";
            this.Sec30MenuItem.Size = new System.Drawing.Size(132, 22);
            this.Sec30MenuItem.Tag = "30";
            this.Sec30MenuItem.Text = "30 Secs";
            this.Sec30MenuItem.Click += new System.EventHandler(this.Sec30MenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.IsFrameworkMenu = false;
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(129, 6);
            // 
            // Minute1MenuItem
            // 
            this.Minute1MenuItem.IsFrameworkMenu = false;
            this.Minute1MenuItem.Name = "Minute1MenuItem";
            this.Minute1MenuItem.Size = new System.Drawing.Size(132, 22);
            this.Minute1MenuItem.Tag = "60";
            this.Minute1MenuItem.Text = "1 Minute";
            this.Minute1MenuItem.Click += new System.EventHandler(this.Minute1MenuItem_Click);
            // 
            // Minute2MenuItem
            // 
            this.Minute2MenuItem.IsFrameworkMenu = false;
            this.Minute2MenuItem.Name = "Minute2MenuItem";
            this.Minute2MenuItem.Size = new System.Drawing.Size(132, 22);
            this.Minute2MenuItem.Tag = "120";
            this.Minute2MenuItem.Text = "2 Minutes";
            this.Minute2MenuItem.Click += new System.EventHandler(this.Minute2MenuItem_Click);
            // 
            // Minute5MenuItem
            // 
            this.Minute5MenuItem.IsFrameworkMenu = false;
            this.Minute5MenuItem.Name = "Minute5MenuItem";
            this.Minute5MenuItem.Size = new System.Drawing.Size(132, 22);
            this.Minute5MenuItem.Tag = "300";
            this.Minute5MenuItem.Text = "5 Minutes";
            this.Minute5MenuItem.Click += new System.EventHandler(this.Minute5MenuItem_Click);
            // 
            // Minute10MenuItem
            // 
            this.Minute10MenuItem.IsFrameworkMenu = false;
            this.Minute10MenuItem.Name = "Minute10MenuItem";
            this.Minute10MenuItem.Size = new System.Drawing.Size(132, 22);
            this.Minute10MenuItem.Tag = "600";
            this.Minute10MenuItem.Text = "10 Minutes";
            this.Minute10MenuItem.Click += new System.EventHandler(this.Minute10MenuItem_Click);
            // 
            // Minute15MenuItem
            // 
            this.Minute15MenuItem.IsFrameworkMenu = false;
            this.Minute15MenuItem.Name = "Minute15MenuItem";
            this.Minute15MenuItem.Size = new System.Drawing.Size(132, 22);
            this.Minute15MenuItem.Tag = "900";
            this.Minute15MenuItem.Text = "15 Minutes";
            this.Minute15MenuItem.Click += new System.EventHandler(this.Minute15MenuItem_Click);
            // 
            // Minute30MenuItem
            // 
            this.Minute30MenuItem.IsFrameworkMenu = false;
            this.Minute30MenuItem.Name = "Minute30MenuItem";
            this.Minute30MenuItem.Size = new System.Drawing.Size(132, 22);
            this.Minute30MenuItem.Tag = "1800";
            this.Minute30MenuItem.Text = "30 Minutes";
            this.Minute30MenuItem.Click += new System.EventHandler(this.Minute30MenuItem_Click);
            // 
            // _theExportButtonSeparator
            // 
            this._theExportButtonSeparator.IsFrameworkMenu = false;
            this._theExportButtonSeparator.Name = "_theExportButtonSeparator";
            this._theExportButtonSeparator.Size = new System.Drawing.Size(6, 27);
            // 
            // _theExportButton
            // 
            this._theExportButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theExportButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theExportButton.Image = ((System.Drawing.Image)(resources.GetObject("_theExportButton.Image")));
            this._theExportButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theExportButton.Name = "_theExportButton";
            this._theExportButton.Size = new System.Drawing.Size(29, 24);
            this._theExportButton.Text = "Export data to the selected format";
            // 
            // _theGraphDetailsSplitter
            // 
            this._theGraphDetailsSplitter.IsFrameworkMenu = false;
            this._theGraphDetailsSplitter.Name = "_theGraphDetailsSplitter";
            this._theGraphDetailsSplitter.Size = new System.Drawing.Size(6, 27);
            // 
            // _theShowGraphButton
            // 
            this._theShowGraphButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theShowGraphButton.Image = ((System.Drawing.Image)(resources.GetObject("_theShowGraphButton.Image")));
            this._theShowGraphButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theShowGraphButton.Name = "_theShowGraphButton";
            this._theShowGraphButton.Size = new System.Drawing.Size(29, 24);
            this._theShowGraphButton.Text = "Show charts";
            // 
            // _theShowDetails
            // 
            this._theShowDetails.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theShowDetails.Image = ((System.Drawing.Image)(resources.GetObject("_theShowDetails.Image")));
            this._theShowDetails.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theShowDetails.Name = "_theShowDetails";
            this._theShowDetails.Size = new System.Drawing.Size(23, 24);
            this._theShowDetails.Text = "Show error and information panels";
            this._theShowDetails.Click += new System.EventHandler(this._theShowDetails_Click);
            // 
            // _thePropertiesSplitter
            // 
            this._thePropertiesSplitter.IsFrameworkMenu = false;
            this._thePropertiesSplitter.Name = "_thePropertiesSplitter";
            this._thePropertiesSplitter.Size = new System.Drawing.Size(6, 27);
            // 
            // _theShowProperties
            // 
            this._theShowProperties.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theShowProperties.Image = ((System.Drawing.Image)(resources.GetObject("_theShowProperties.Image")));
            this._theShowProperties.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theShowProperties.Name = "_theShowProperties";
            this._theShowProperties.Size = new System.Drawing.Size(23, 24);
            this._theShowProperties.Text = "Show Properties";
            this._theShowProperties.Click += new System.EventHandler(this._theShowProperties_Click);
            // 
            // _theHelpButton
            // 
            this._theHelpButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theHelpButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theHelpButton.Image = ((System.Drawing.Image)(resources.GetObject("_theHelpButton.Image")));
            this._theHelpButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theHelpButton.Name = "_theHelpButton";
            this._theHelpButton.Size = new System.Drawing.Size(23, 24);
            this._theHelpButton.Text = "_theHelpButton";
            this._theHelpButton.ToolTipText = "Help";
            this._theHelpButton.Click += new System.EventHandler(this._theHelpButton_Click);
            // 
            // _theRowCount
            // 
            this._theRowCount.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theRowCount.AutoSize = false;
            this._theRowCount.Name = "_theRowCount";
            this._theRowCount.Size = new System.Drawing.Size(50, 25);
            this._theRowCount.Text = "500";
            this._theRowCount.ToolTipText = "Max rows to be returned";
            this._theRowCount.Leave += new System.EventHandler(this._theRowCount_Leave);
            this._theRowCount.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this._theRowCount_KeyPress);
            // 
            // _theRowCountLabel
            // 
            this._theRowCountLabel.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theRowCountLabel.Name = "_theRowCountLabel";
            this._theRowCountLabel.Size = new System.Drawing.Size(44, 15);
            this._theRowCountLabel.Text = "Rows : ";
            // 
            // _theCustomToolStripButtonSeparator
            // 
            this._theCustomToolStripButtonSeparator.IsFrameworkMenu = false;
            this._theCustomToolStripButtonSeparator.Name = "_theCustomToolStripButtonSeparator";
            this._theCustomToolStripButtonSeparator.Size = new System.Drawing.Size(6, 25);
            // 
            // _theWidgetCanvas
            // 
            this._theWidgetCanvas.ActiveWidget = null;
            this._theWidgetCanvas.AllowDelete = true;
            this._theWidgetCanvas.AllowDrop = true;
            this._theWidgetCanvas.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetCanvas.LayoutManager = null;
            this._theWidgetCanvas.Location = new System.Drawing.Point(0, 0);
            this._theWidgetCanvas.LockBackColor = System.Drawing.SystemColors.Control;
            this._theWidgetCanvas.Name = "_theWidgetCanvas";
            this._theWidgetCanvas.Size = new System.Drawing.Size(838, 607);
            this._theWidgetCanvas.TabIndex = 0;
            this._theWidgetCanvas.ThePersistenceKey = null;
            this._theWidgetCanvas.UnlockBackColor = System.Drawing.Color.Azure;
            this._theWidgetCanvas.ViewName = null;
            this._theWidgetCanvas.ViewNum = 0;
            this._theWidgetCanvas.ViewText = null;
            this._theWidgetCanvas.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("_theWidgetCanvas.WidgetsModel")));
            // 
            // GenericUniversalWidget
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this._theToolStrip);
            this.Controls.Add(this._theWidgetCanvas);
            this.Name = "GenericUniversalWidget";
            this.Size = new System.Drawing.Size(838, 607);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionSplitContainer1.Panel1.ResumeLayout(false);
            this.TrafodionSplitContainer1.Panel2.ResumeLayout(false);
            this.TrafodionSplitContainer1.ResumeLayout(false);
            this.TrafodionSplitContainer2.Panel1.ResumeLayout(false);
            this.TrafodionSplitContainer2.Panel2.ResumeLayout(false);
            this.TrafodionSplitContainer2.ResumeLayout(false);
            this._theDisplayPanel.ResumeLayout(false);
            this._theLowerTabControl.ResumeLayout(false);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this._theToolStrip.ResumeLayout(false);
            this._theToolStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.WidgetCanvas _theWidgetCanvas;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer TrafodionSplitContainer1;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer TrafodionSplitContainer2;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _theLowerTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theParameterTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theDocumentationTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theErrorTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionStatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel _theStatusLabel;
        private System.Windows.Forms.ToolStripProgressBar _theStatusProgressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theConfigurationContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theDisplayPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip _theToolStrip;
        private System.Windows.Forms.ToolStripComboBox _theCatalogs;
        private System.Windows.Forms.ToolStripLabel _theCatalogLabel;
        private System.Windows.Forms.ToolStripLabel _theSchemaLabel;
        private System.Windows.Forms.ToolStripComboBox _theSchemas;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator _theSplitter1;
        private System.Windows.Forms.ToolStripButton _theShowProperties;
        private System.Windows.Forms.ToolStripButton _theShowDetails;
        private System.Windows.Forms.ToolStripButton _theRefreshButton;
        private System.Windows.Forms.ToolStripLabel _theConnectionLabel;
        private System.Windows.Forms.ToolStripComboBox _theConnections;
        private System.Windows.Forms.ToolStripTextBox _theRowCount;
        private System.Windows.Forms.ToolStripLabel _theRowCountLabel;
        private System.Windows.Forms.ToolStripButton _theStopQuery;
        private System.Windows.Forms.ToolStripStatusLabel _theCancelQueryLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator _theGraphDetailsSplitter;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator _thePropertiesSplitter;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator _theCustomToolStripButtonSeparator;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theOutputTab;
        private System.Windows.Forms.ToolStripStatusLabel _theRefreshLabel;
        private System.Windows.Forms.ToolStripButton _theRefreshTimerButton;
        private System.Windows.Forms.ToolStripDropDownButton _theTimerSetupButton;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem Sec30MenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem Minute1MenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator toolStripSeparator1;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem Minute2MenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem Minute5MenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem Minute10MenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem Minute15MenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripMenuItem Minute30MenuItem;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStripSeparator _theExportButtonSeparator;
        private System.Windows.Forms.ToolStripDropDownButton _theExportButton;
        private System.Windows.Forms.ToolStripButton _theHelpButton;
        private System.Windows.Forms.ToolStripDropDownButton _theShowGraphButton;

    }
}
