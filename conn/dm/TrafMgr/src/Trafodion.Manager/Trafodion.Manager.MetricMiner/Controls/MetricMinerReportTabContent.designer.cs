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
namespace Trafodion.Manager.MetricMiner.Controls
{
    partial class MetricMinerReportTabContent
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
            Trafodion.Manager.Framework.Controls.TrafodionLabel label2;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label6;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MetricMinerReportTabContent));
            this._theSplitPanel = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theQuerySplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theQueryInputControl = new Trafodion.Manager.MetricMiner.Controls.QueryInputControl();
            this._theButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theUWPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theHeaderPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSelectorPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSchemasPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSchemasComboBox = new Trafodion.Manager.DatabaseArea.Controls.SchemasComboBox();
            this._theCatalogsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theCatalogsComboBox = new Trafodion.Manager.DatabaseArea.Controls.CatalogsComboBox();
            this._theSystemsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSystemLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSystemsCombo = new Trafodion.Manager.Framework.Connections.Controls.MySystemsComboBox();
            this._theShowHideQuery = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theQueryPropertyTabPanel = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._theDescriptionTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theDescriptionDisplayUserControl = new Trafodion.Manager.MetricMiner.Controls.DescriptionDisplayUserControl();
            this._theStatusTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theStatusDisplayUserControl = new Trafodion.Manager.MetricMiner.Controls.StatusDisplayUserControl();
            this._theParameterTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._thePrametersDisplayUserControl = new Trafodion.Manager.MetricMiner.Controls.PrametersDisplayUserControl();
            this._theRowDetailsTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theRowDisplayPanel = new Trafodion.Manager.MetricMiner.Controls.TrafodionIGridHtmlRowDisplay();
            label2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSplitPanel.Panel1.SuspendLayout();
            this._theSplitPanel.Panel2.SuspendLayout();
            this._theSplitPanel.SuspendLayout();
            this._theQuerySplitContainer.Panel1.SuspendLayout();
            this._theQuerySplitContainer.Panel2.SuspendLayout();
            this._theQuerySplitContainer.SuspendLayout();
            this._theHeaderPanel.SuspendLayout();
            this._theSelectorPanel.SuspendLayout();
            this._theSchemasPanel.SuspendLayout();
            this._theCatalogsPanel.SuspendLayout();
            this._theSystemsPanel.SuspendLayout();
            this._theQueryPropertyTabPanel.SuspendLayout();
            this._theDescriptionTab.SuspendLayout();
            this._theStatusTab.SuspendLayout();
            this._theParameterTab.SuspendLayout();
            this._theRowDetailsTab.SuspendLayout();
            this.SuspendLayout();
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Font = new System.Drawing.Font("Tahoma", 8F);
            label2.Location = new System.Drawing.Point(5, 6);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(48, 13);
            label2.TabIndex = 3;
            label2.Text = "Schema:";
            label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // label6
            // 
            label6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            label6.AutoSize = true;
            label6.Font = new System.Drawing.Font("Tahoma", 8F);
            label6.Location = new System.Drawing.Point(3, 7);
            label6.Name = "label6";
            label6.Size = new System.Drawing.Size(48, 13);
            label6.TabIndex = 4;
            label6.Text = "Catalog:";
            // 
            // _theSplitPanel
            // 
            this._theSplitPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSplitPanel.Location = new System.Drawing.Point(0, 0);
            this._theSplitPanel.Name = "_theSplitPanel";
            // 
            // _theSplitPanel.Panel1
            // 
            this._theSplitPanel.Panel1.Controls.Add(this._theQuerySplitContainer);
            this._theSplitPanel.Panel1.Controls.Add(this._theHeaderPanel);
            // 
            // _theSplitPanel.Panel2
            // 
            this._theSplitPanel.Panel2.Controls.Add(this._theQueryPropertyTabPanel);
            this._theSplitPanel.Size = new System.Drawing.Size(1114, 447);
            this._theSplitPanel.SplitterDistance = 840;
            this._theSplitPanel.SplitterWidth = 9;
            this._theSplitPanel.TabIndex = 0;
            // 
            // _theQuerySplitContainer
            // 
            this._theQuerySplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQuerySplitContainer.Location = new System.Drawing.Point(0, 51);
            this._theQuerySplitContainer.Name = "_theQuerySplitContainer";
            this._theQuerySplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _theQuerySplitContainer.Panel1
            // 
            this._theQuerySplitContainer.Panel1.Controls.Add(this._theQueryInputControl);
            this._theQuerySplitContainer.Panel1.Controls.Add(this._theButtonPanel);
            // 
            // _theQuerySplitContainer.Panel2
            // 
            this._theQuerySplitContainer.Panel2.Controls.Add(this._theUWPanel);
            this._theQuerySplitContainer.Size = new System.Drawing.Size(840, 396);
            this._theQuerySplitContainer.SplitterDistance = 145;
            this._theQuerySplitContainer.SplitterWidth = 9;
            this._theQuerySplitContainer.TabIndex = 2;
            // 
            // _theQueryInputControl
            // 
            this._theQueryInputControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryInputControl.Location = new System.Drawing.Point(0, 0);
            this._theQueryInputControl.Name = "_theQueryInputControl";
            this._theQueryInputControl.QueryText = "";
            this._theQueryInputControl.Size = new System.Drawing.Size(840, 145);
            this._theQueryInputControl.TabIndex = 0;
            this._theQueryInputControl.OnQueryTextChanged += new Trafodion.Manager.MetricMiner.Controls.QueryInputControl.QueryInputTextChanged(this._theQueryInputControl_OnQueryTextChanged);
            // 
            // _theButtonPanel
            // 
            this._theButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theButtonPanel.Location = new System.Drawing.Point(0, 125);
            this._theButtonPanel.Name = "_theButtonPanel";
            this._theButtonPanel.Size = new System.Drawing.Size(481, 33);
            this._theButtonPanel.TabIndex = 1;
            this._theButtonPanel.Visible = false;
            // 
            // _theUWPanel
            // 
            this._theUWPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theUWPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theUWPanel.Location = new System.Drawing.Point(0, 0);
            this._theUWPanel.Name = "_theUWPanel";
            this._theUWPanel.Size = new System.Drawing.Size(840, 242);
            this._theUWPanel.TabIndex = 0;
            // 
            // _theHeaderPanel
            // 
            this._theHeaderPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theHeaderPanel.Controls.Add(this._theSelectorPanel);
            this._theHeaderPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theHeaderPanel.Location = new System.Drawing.Point(0, 0);
            this._theHeaderPanel.Name = "_theHeaderPanel";
            this._theHeaderPanel.Size = new System.Drawing.Size(840, 51);
            this._theHeaderPanel.TabIndex = 1;
            // 
            // _theSelectorPanel
            // 
            this._theSelectorPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSelectorPanel.Controls.Add(this._theSchemasPanel);
            this._theSelectorPanel.Controls.Add(this._theCatalogsPanel);
            this._theSelectorPanel.Controls.Add(this._theSystemsPanel);
            this._theSelectorPanel.Controls.Add(this._theShowHideQuery);
            this._theSelectorPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theSelectorPanel.Location = new System.Drawing.Point(0, 26);
            this._theSelectorPanel.Name = "_theSelectorPanel";
            this._theSelectorPanel.Size = new System.Drawing.Size(840, 25);
            this._theSelectorPanel.TabIndex = 3;
            // 
            // _theSchemasPanel
            // 
            this._theSchemasPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSchemasPanel.Controls.Add(label2);
            this._theSchemasPanel.Controls.Add(this._theSchemasComboBox);
            this._theSchemasPanel.Location = new System.Drawing.Point(397, 0);
            this._theSchemasPanel.Name = "_theSchemasPanel";
            this._theSchemasPanel.Size = new System.Drawing.Size(294, 27);
            this._theSchemasPanel.TabIndex = 6;
            // 
            // _theSchemasComboBox
            // 
            this._theSchemasComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSchemasComboBox.DropDownHeight = 400;
            this._theSchemasComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theSchemasComboBox.DropDownWidth = 200;
            this._theSchemasComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSchemasComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theSchemasComboBox.FormattingEnabled = true;
            this._theSchemasComboBox.IntegralHeight = false;
            this._theSchemasComboBox.Location = new System.Drawing.Point(57, 3);
            this._theSchemasComboBox.Name = "_theSchemasComboBox";
            this._theSchemasComboBox.SelectedTrafodionObject = null;
            this._theSchemasComboBox.SelectedTrafodionSchema = null;
            this._theSchemasComboBox.Size = new System.Drawing.Size(234, 21);
            this._theSchemasComboBox.Sorted = true;
            this._theSchemasComboBox.TabIndex = 2;
            this._theSchemasComboBox.TheTrafodionCatalog = null;
            this._theSchemasComboBox.SelectedIndexChanged += new System.EventHandler(this._theSchemasComboBox_SelectedIndexChanged);
            // 
            // _theCatalogsPanel
            // 
            this._theCatalogsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theCatalogsPanel.Controls.Add(label6);
            this._theCatalogsPanel.Controls.Add(this._theCatalogsComboBox);
            this._theCatalogsPanel.Location = new System.Drawing.Point(188, 0);
            this._theCatalogsPanel.Name = "_theCatalogsPanel";
            this._theCatalogsPanel.Size = new System.Drawing.Size(206, 27);
            this._theCatalogsPanel.TabIndex = 4;
            // 
            // _theCatalogsComboBox
            // 
            this._theCatalogsComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theCatalogsComboBox.DropDownHeight = 400;
            this._theCatalogsComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theCatalogsComboBox.DropDownWidth = 200;
            this._theCatalogsComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCatalogsComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theCatalogsComboBox.FormattingEnabled = true;
            this._theCatalogsComboBox.IntegralHeight = false;
            this._theCatalogsComboBox.Location = new System.Drawing.Point(54, 3);
            this._theCatalogsComboBox.Name = "_theCatalogsComboBox";
            this._theCatalogsComboBox.SelectedTrafodionCatalog = null;
            this._theCatalogsComboBox.SelectedTrafodionObject = null;
            this._theCatalogsComboBox.Size = new System.Drawing.Size(149, 21);
            this._theCatalogsComboBox.Sorted = true;
            this._theCatalogsComboBox.TabIndex = 3;
            this._theCatalogsComboBox.TheTrafodionSystem = null;
            this._theCatalogsComboBox.SelectedIndexChanged += new System.EventHandler(this._theCatalogsComboBox_SelectedIndexChanged);
            // 
            // _theSystemsPanel
            // 
            this._theSystemsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSystemsPanel.Controls.Add(this._theSystemLabel);
            this._theSystemsPanel.Controls.Add(this._theSystemsCombo);
            this._theSystemsPanel.Location = new System.Drawing.Point(3, 0);
            this._theSystemsPanel.Name = "_theSystemsPanel";
            this._theSystemsPanel.Size = new System.Drawing.Size(179, 27);
            this._theSystemsPanel.TabIndex = 3;
            // 
            // _theSystemLabel
            // 
            this._theSystemLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theSystemLabel.AutoSize = true;
            this._theSystemLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theSystemLabel.Location = new System.Drawing.Point(3, 6);
            this._theSystemLabel.Name = "_theSystemLabel";
            this._theSystemLabel.Size = new System.Drawing.Size(46, 13);
            this._theSystemLabel.TabIndex = 3;
            this._theSystemLabel.Text = "System:";
            this._theSystemLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // _theSystemsCombo
            // 
            this._theSystemsCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSystemsCombo.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this._theSystemsCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theSystemsCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSystemsCombo.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theSystemsCombo.FormattingEnabled = true;
            this._theSystemsCombo.IsLoading = false;
            this._theSystemsCombo.Location = new System.Drawing.Point(51, 3);
            this._theSystemsCombo.Name = "_theSystemsCombo";
            this._theSystemsCombo.SelectedCatalogName = null;
            this._theSystemsCombo.SelectedConnectionDefinition = null;
            this._theSystemsCombo.SelectedSchemaName = null;
            this._theSystemsCombo.Size = new System.Drawing.Size(125, 21);
            this._theSystemsCombo.Sorted = true;
            this._theSystemsCombo.TabIndex = 1;
            this._theSystemsCombo.SelectedIndexChanged += new System.EventHandler(this._theSystemsCombo_SelectedIndexChanged);
            // 
            // _theShowHideQuery
            // 
            this._theShowHideQuery.Dock = System.Windows.Forms.DockStyle.Right;
            this._theShowHideQuery.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theShowHideQuery.Image = ((System.Drawing.Image)(resources.GetObject("_theShowHideQuery.Image")));
            this._theShowHideQuery.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._theShowHideQuery.Location = new System.Drawing.Point(740, 0);
            this._theShowHideQuery.Name = "_theShowHideQuery";
            this._theShowHideQuery.Size = new System.Drawing.Size(100, 25);
            this._theShowHideQuery.TabIndex = 0;
            this._theShowHideQuery.Text = "Show Query";
            this._theShowHideQuery.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theShowHideQuery.Click += new System.EventHandler(this._theShowHideQuery_Click);
            // 
            // _theQueryPropertyTabPanel
            // 
            this._theQueryPropertyTabPanel.Controls.Add(this._theDescriptionTab);
            this._theQueryPropertyTabPanel.Controls.Add(this._theStatusTab);
            this._theQueryPropertyTabPanel.Controls.Add(this._theParameterTab);
            this._theQueryPropertyTabPanel.Controls.Add(this._theRowDetailsTab);
            this._theQueryPropertyTabPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryPropertyTabPanel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theQueryPropertyTabPanel.HotTrack = true;
            this._theQueryPropertyTabPanel.Location = new System.Drawing.Point(0, 0);
            this._theQueryPropertyTabPanel.Multiline = true;
            this._theQueryPropertyTabPanel.Name = "_theQueryPropertyTabPanel";
            this._theQueryPropertyTabPanel.Padding = new System.Drawing.Point(10, 5);
            this._theQueryPropertyTabPanel.SelectedIndex = 0;
            this._theQueryPropertyTabPanel.Size = new System.Drawing.Size(265, 447);
            this._theQueryPropertyTabPanel.TabIndex = 0;
            // 
            // _theDescriptionTab
            // 
            this._theDescriptionTab.Controls.Add(this._theDescriptionDisplayUserControl);
            this._theDescriptionTab.Location = new System.Drawing.Point(4, 48);
            this._theDescriptionTab.Name = "_theDescriptionTab";
            this._theDescriptionTab.Size = new System.Drawing.Size(257, 395);
            this._theDescriptionTab.TabIndex = 2;
            this._theDescriptionTab.Text = "Description";
            this._theDescriptionTab.UseVisualStyleBackColor = true;
            // 
            // _theDescriptionDisplayUserControl
            // 
            this._theDescriptionDisplayUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theDescriptionDisplayUserControl.Location = new System.Drawing.Point(0, 0);
            this._theDescriptionDisplayUserControl.Name = "_theDescriptionDisplayUserControl";
            this._theDescriptionDisplayUserControl.Size = new System.Drawing.Size(257, 395);
            this._theDescriptionDisplayUserControl.TabIndex = 0;
            // 
            // _theStatusTab
            // 
            this._theStatusTab.Controls.Add(this._theStatusDisplayUserControl);
            this._theStatusTab.Location = new System.Drawing.Point(4, 26);
            this._theStatusTab.Name = "_theStatusTab";
            this._theStatusTab.Size = new System.Drawing.Size(257, 417);
            this._theStatusTab.TabIndex = 3;
            this._theStatusTab.Text = "Status";
            this._theStatusTab.UseVisualStyleBackColor = true;
            // 
            // _theStatusDisplayUserControl
            // 
            this._theStatusDisplayUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theStatusDisplayUserControl.Location = new System.Drawing.Point(0, 0);
            this._theStatusDisplayUserControl.Name = "_theStatusDisplayUserControl";
            this._theStatusDisplayUserControl.Size = new System.Drawing.Size(257, 417);
            this._theStatusDisplayUserControl.TabIndex = 0;
            // 
            // _theParameterTab
            // 
            this._theParameterTab.Controls.Add(this._thePrametersDisplayUserControl);
            this._theParameterTab.Location = new System.Drawing.Point(4, 26);
            this._theParameterTab.Name = "_theParameterTab";
            this._theParameterTab.Padding = new System.Windows.Forms.Padding(3);
            this._theParameterTab.Size = new System.Drawing.Size(257, 417);
            this._theParameterTab.TabIndex = 1;
            this._theParameterTab.Text = "Parameters";
            this._theParameterTab.UseVisualStyleBackColor = true;
            // 
            // _thePrametersDisplayUserControl
            // 
            this._thePrametersDisplayUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePrametersDisplayUserControl.Location = new System.Drawing.Point(3, 3);
            this._thePrametersDisplayUserControl.Name = "_thePrametersDisplayUserControl";
            this._thePrametersDisplayUserControl.Size = new System.Drawing.Size(251, 411);
            this._thePrametersDisplayUserControl.TabIndex = 0;
            // 
            // _theRowDetailsTab
            // 
            this._theRowDetailsTab.Controls.Add(this._theRowDisplayPanel);
            this._theRowDetailsTab.Location = new System.Drawing.Point(4, 48);
            this._theRowDetailsTab.Name = "_theRowDetailsTab";
            this._theRowDetailsTab.Padding = new System.Windows.Forms.Padding(3);
            this._theRowDetailsTab.Size = new System.Drawing.Size(257, 395);
            this._theRowDetailsTab.TabIndex = 0;
            this._theRowDetailsTab.Text = "Row Details";
            this._theRowDetailsTab.UseVisualStyleBackColor = true;
            // 
            // _theRowDisplayPanel
            // 
            this._theRowDisplayPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theRowDisplayPanel.Location = new System.Drawing.Point(3, 3);
            this._theRowDisplayPanel.Name = "_theRowDisplayPanel";
            this._theRowDisplayPanel.Size = new System.Drawing.Size(251, 389);
            this._theRowDisplayPanel.TabIndex = 0;
            // 
            // MetricMinerReportTabContent
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theSplitPanel);
            this.Name = "MetricMinerReportTabContent";
            this.Size = new System.Drawing.Size(1114, 447);
            this._theSplitPanel.Panel1.ResumeLayout(false);
            this._theSplitPanel.Panel2.ResumeLayout(false);
            this._theSplitPanel.ResumeLayout(false);
            this._theQuerySplitContainer.Panel1.ResumeLayout(false);
            this._theQuerySplitContainer.Panel2.ResumeLayout(false);
            this._theQuerySplitContainer.ResumeLayout(false);
            this._theHeaderPanel.ResumeLayout(false);
            this._theSelectorPanel.ResumeLayout(false);
            this._theSchemasPanel.ResumeLayout(false);
            this._theSchemasPanel.PerformLayout();
            this._theCatalogsPanel.ResumeLayout(false);
            this._theCatalogsPanel.PerformLayout();
            this._theSystemsPanel.ResumeLayout(false);
            this._theSystemsPanel.PerformLayout();
            this._theQueryPropertyTabPanel.ResumeLayout(false);
            this._theDescriptionTab.ResumeLayout(false);
            this._theStatusTab.ResumeLayout(false);
            this._theParameterTab.ResumeLayout(false);
            this._theRowDetailsTab.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theSplitPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _theQueryPropertyTabPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theRowDetailsTab;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theParameterTab;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theDescriptionTab;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theHeaderPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theShowHideQuery;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theUWPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theQuerySplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theButtonPanel;
        private Trafodion.Manager.Framework.Connections.Controls.MySystemsComboBox _theSystemsCombo;
        private DescriptionDisplayUserControl _theDescriptionDisplayUserControl;
        private TrafodionIGridHtmlRowDisplay _theRowDisplayPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theSelectorPanel;
        private PrametersDisplayUserControl _thePrametersDisplayUserControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theStatusTab;
        private StatusDisplayUserControl _theStatusDisplayUserControl;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theSystemsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theSystemLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theCatalogsPanel;
        private Trafodion.Manager.DatabaseArea.Controls.CatalogsComboBox _theCatalogsComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theSchemasPanel;
        private Trafodion.Manager.DatabaseArea.Controls.SchemasComboBox _theSchemasComboBox;
        protected QueryInputControl _theQueryInputControl;
    }
}
