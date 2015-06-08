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

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryUserControl
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
            Trafodion.Manager.Framework.Controls.TrafodionPanel panel2;
            Trafodion.Manager.Framework.Controls.TrafodionPanel _theNameCatalogsAndMaxRowsPanel;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label6;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label4;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label3;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
            Trafodion.Manager.Framework.Controls.TrafodionLabel label2;
            Trafodion.Manager.Framework.Controls.TrafodionLabel rowsPerPageLabel;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(QueryUserControl));
            this._theUpdateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theExplainButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theExecuteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCatalogsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theCatalogsComboBox = new Trafodion.Manager.DatabaseArea.Controls.CatalogsComboBox();
            this._thaNamePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theMaxRowsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theMaxRowsTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theSystemsAndSchemasPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSchemasPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theSchemasComboBox = new Trafodion.Manager.DatabaseArea.Controls.SchemasComboBox();
            this._theSystemsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theMySystemsComboBox = new Trafodion.Manager.Framework.Connections.Controls.MySystemsComboBox();
            this.headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theHelpStripButton = new System.Windows.Forms.ToolStripButton();
            this.theRowCountsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theRowsPerPagePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._includeTableStatsCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._rowsPerPageTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionToolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theStatementSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theQueryInputControl = new Trafodion.Manager.DatabaseArea.Queries.Controls.QueryInputControl();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theCQSettingControl = new Trafodion.Manager.DatabaseArea.Queries.Controls.CQSettingControl();
            panel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            _theNameCatalogsAndMaxRowsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            label6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            label2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            rowsPerPageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            panel2.SuspendLayout();
            _theNameCatalogsAndMaxRowsPanel.SuspendLayout();
            this._theCatalogsPanel.SuspendLayout();
            this._thaNamePanel.SuspendLayout();
            this._theMaxRowsPanel.SuspendLayout();
            this._theSystemsAndSchemasPanel.SuspendLayout();
            this._theSchemasPanel.SuspendLayout();
            this._theSystemsPanel.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this.theRowCountsPanel.SuspendLayout();
            this.theRowsPerPagePanel.SuspendLayout();
            this._theStatementSplitContainer.Panel1.SuspendLayout();
            this._theStatementSplitContainer.Panel2.SuspendLayout();
            this._theStatementSplitContainer.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel2
            // 
            panel2.BackColor = System.Drawing.Color.WhiteSmoke;
            panel2.Controls.Add(this._theUpdateButton);
            panel2.Controls.Add(this._theAddButton);
            panel2.Controls.Add(this._theExplainButton);
            panel2.Controls.Add(this._theExecuteButton);
            panel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            panel2.Location = new System.Drawing.Point(0, 350);
            panel2.Name = "panel2";
            panel2.Size = new System.Drawing.Size(857, 30);
            panel2.TabIndex = 0;
            // 
            // _theUpdateButton
            // 
            this._theUpdateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUpdateButton.Location = new System.Drawing.Point(81, 3);
            this._theUpdateButton.Name = "_theUpdateButton";
            this._theUpdateButton.Size = new System.Drawing.Size(75, 23);
            this._theUpdateButton.TabIndex = 2;
            this._theUpdateButton.Text = "&Update";
            this._theUpdateButton.UseVisualStyleBackColor = true;
            this._theUpdateButton.Click += new System.EventHandler(this.TheUpdateButtonClick);
            // 
            // _theAddButton
            // 
            this._theAddButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAddButton.Location = new System.Drawing.Point(0, 3);
            this._theAddButton.Name = "_theAddButton";
            this._theAddButton.Size = new System.Drawing.Size(75, 23);
            this._theAddButton.TabIndex = 1;
            this._theAddButton.Text = "&Add";
            this._theAddButton.UseVisualStyleBackColor = true;
            this._theAddButton.Click += new System.EventHandler(this.TheAddButtonClick);
            // 
            // _theExplainButton
            // 
            this._theExplainButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theExplainButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theExplainButton.Location = new System.Drawing.Point(701, 3);
            this._theExplainButton.Name = "_theExplainButton";
            this._theExplainButton.Size = new System.Drawing.Size(75, 23);
            this._theExplainButton.TabIndex = 3;
            this._theExplainButton.Text = "Ex&plain";
            this._theExplainButton.UseVisualStyleBackColor = true;
            this._theExplainButton.Click += new System.EventHandler(this.TheExplainButtonClick);
            // 
            // _theExecuteButton
            // 
            this._theExecuteButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theExecuteButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theExecuteButton.Location = new System.Drawing.Point(782, 3);
            this._theExecuteButton.Name = "_theExecuteButton";
            this._theExecuteButton.Size = new System.Drawing.Size(75, 23);
            this._theExecuteButton.TabIndex = 5;
            this._theExecuteButton.Text = "E&xecute";
            this._theExecuteButton.UseVisualStyleBackColor = true;
            this._theExecuteButton.Click += new System.EventHandler(this.TheExecuteButtonClick);
            // 
            // _theNameCatalogsAndMaxRowsPanel
            // 
            _theNameCatalogsAndMaxRowsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            _theNameCatalogsAndMaxRowsPanel.Controls.Add(this._theCatalogsPanel);
            _theNameCatalogsAndMaxRowsPanel.Controls.Add(this._thaNamePanel);
            _theNameCatalogsAndMaxRowsPanel.Dock = System.Windows.Forms.DockStyle.Top;
            _theNameCatalogsAndMaxRowsPanel.Location = new System.Drawing.Point(0, 26);
            _theNameCatalogsAndMaxRowsPanel.Name = "_theNameCatalogsAndMaxRowsPanel";
            _theNameCatalogsAndMaxRowsPanel.Size = new System.Drawing.Size(857, 29);
            _theNameCatalogsAndMaxRowsPanel.TabIndex = 1;
            // 
            // _theCatalogsPanel
            // 
            this._theCatalogsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theCatalogsPanel.Controls.Add(label6);
            this._theCatalogsPanel.Controls.Add(this._theCatalogsComboBox);
            this._theCatalogsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theCatalogsPanel.Location = new System.Drawing.Point(210, 0);
            this._theCatalogsPanel.Name = "_theCatalogsPanel";
            this._theCatalogsPanel.Size = new System.Drawing.Size(647, 29);
            this._theCatalogsPanel.TabIndex = 2;
            this._theCatalogsPanel.Visible = false;
            // 
            // label6
            // 
            label6.AutoSize = true;
            label6.Font = new System.Drawing.Font("Tahoma", 8F);
            label6.Location = new System.Drawing.Point(1, 8);
            label6.Name = "label6";
            label6.Size = new System.Drawing.Size(48, 13);
            label6.TabIndex = 0;
            label6.Text = "Catalog:";
            // 
            // _theCatalogsComboBox
            // 
            this._theCatalogsComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theCatalogsComboBox.DropDownHeight = 400;
            this._theCatalogsComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theCatalogsComboBox.DropDownWidth = 200;
            this._theCatalogsComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCatalogsComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theCatalogsComboBox.FormattingEnabled = true;
            this._theCatalogsComboBox.IntegralHeight = false;
            this._theCatalogsComboBox.Location = new System.Drawing.Point(74, 4);
            this._theCatalogsComboBox.Name = "_theCatalogsComboBox";
            this._theCatalogsComboBox.SelectedTrafodionCatalog = null;
            this._theCatalogsComboBox.SelectedTrafodionObject = null;
            this._theCatalogsComboBox.Size = new System.Drawing.Size(570, 21);
            this._theCatalogsComboBox.Sorted = true;
            this._theCatalogsComboBox.TabIndex = 1;
            this._theCatalogsComboBox.TheTrafodionSystem = null;
            this._theCatalogsComboBox.SelectedIndexChanged += new System.EventHandler(this.TheCatalogsComboBoxSelectedIndexChanged);
            // 
            // _thaNamePanel
            // 
            this._thaNamePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._thaNamePanel.Controls.Add(this._theNameTextBox);
            this._thaNamePanel.Controls.Add(label4);
            this._thaNamePanel.Dock = System.Windows.Forms.DockStyle.Left;
            this._thaNamePanel.Location = new System.Drawing.Point(0, 0);
            this._thaNamePanel.Name = "_thaNamePanel";
            this._thaNamePanel.Size = new System.Drawing.Size(210, 29);
            this._thaNamePanel.TabIndex = 1;
            // 
            // _theNameTextBox
            // 
            this._theNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theNameTextBox.Location = new System.Drawing.Point(66, 5);
            this._theNameTextBox.Name = "_theNameTextBox";
            this._theNameTextBox.Size = new System.Drawing.Size(140, 21);
            this._theNameTextBox.TabIndex = 1;
            this._theNameTextBox.TextChanged += new System.EventHandler(this.TheNameTextBoxTextChanged);
            // 
            // label4
            // 
            label4.AutoSize = true;
            label4.Font = new System.Drawing.Font("Tahoma", 8F);
            label4.Location = new System.Drawing.Point(3, 8);
            label4.Name = "label4";
            label4.Size = new System.Drawing.Size(38, 13);
            label4.TabIndex = 0;
            label4.Text = "Name:";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Font = new System.Drawing.Font("Tahoma", 8F);
            label3.Location = new System.Drawing.Point(3, 6);
            label3.Name = "label3";
            label3.Size = new System.Drawing.Size(60, 13);
            label3.TabIndex = 1;
            label3.Text = "Max Rows:";
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Font = new System.Drawing.Font("Tahoma", 8F);
            label1.Location = new System.Drawing.Point(3, 8);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(46, 13);
            label1.TabIndex = 1;
            label1.Text = "System:";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Font = new System.Drawing.Font("Tahoma", 8F);
            label2.Location = new System.Drawing.Point(1, 8);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(48, 13);
            label2.TabIndex = 0;
            label2.Text = "Schema:";
            // 
            // rowsPerPageLabel
            // 
            rowsPerPageLabel.AutoSize = true;
            rowsPerPageLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            rowsPerPageLabel.Location = new System.Drawing.Point(1, 6);
            rowsPerPageLabel.Name = "rowsPerPageLabel";
            rowsPerPageLabel.Size = new System.Drawing.Size(71, 13);
            rowsPerPageLabel.TabIndex = 1;
            rowsPerPageLabel.Text = "Rows / Page:";
            // 
            // _theMaxRowsPanel
            // 
            this._theMaxRowsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMaxRowsPanel.Controls.Add(label3);
            this._theMaxRowsPanel.Controls.Add(this._theMaxRowsTextBox);
            this._theMaxRowsPanel.Dock = System.Windows.Forms.DockStyle.Left;
            this._theMaxRowsPanel.Location = new System.Drawing.Point(0, 0);
            this._theMaxRowsPanel.Name = "_theMaxRowsPanel";
            this._theMaxRowsPanel.Size = new System.Drawing.Size(210, 28);
            this._theMaxRowsPanel.TabIndex = 0;
            // 
            // _theMaxRowsTextBox
            // 
            this._theMaxRowsTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theMaxRowsTextBox.Location = new System.Drawing.Point(66, 4);
            this._theMaxRowsTextBox.Name = "_theMaxRowsTextBox";
            this._theMaxRowsTextBox.Size = new System.Drawing.Size(140, 21);
            this._theMaxRowsTextBox.TabIndex = 1;
            this._theMaxRowsTextBox.TextChanged += new System.EventHandler(this.TheMaxRowsTextBoxTextChanged);
            this._theMaxRowsTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this._theMaxRowsTextBox_KeyPress);
            this._theMaxRowsTextBox.KeyUp += new System.Windows.Forms.KeyEventHandler(this._theMaxRowsTextBox_KeyUp);
            // 
            // _theSystemsAndSchemasPanel
            // 
            this._theSystemsAndSchemasPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSystemsAndSchemasPanel.Controls.Add(this._theSchemasPanel);
            this._theSystemsAndSchemasPanel.Controls.Add(this._theSystemsPanel);
            this._theSystemsAndSchemasPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theSystemsAndSchemasPanel.Location = new System.Drawing.Point(0, 55);
            this._theSystemsAndSchemasPanel.Name = "_theSystemsAndSchemasPanel";
            this._theSystemsAndSchemasPanel.Size = new System.Drawing.Size(857, 29);
            this._theSystemsAndSchemasPanel.TabIndex = 14;
            // 
            // _theSchemasPanel
            // 
            this._theSchemasPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSchemasPanel.Controls.Add(this._theSchemasComboBox);
            this._theSchemasPanel.Controls.Add(label2);
            this._theSchemasPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theSchemasPanel.Location = new System.Drawing.Point(210, 0);
            this._theSchemasPanel.Name = "_theSchemasPanel";
            this._theSchemasPanel.Size = new System.Drawing.Size(647, 29);
            this._theSchemasPanel.TabIndex = 0;
            this._theSchemasPanel.Visible = false;
            // 
            // _theSchemasComboBox
            // 
            this._theSchemasComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSchemasComboBox.DropDownHeight = 400;
            this._theSchemasComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theSchemasComboBox.DropDownWidth = 200;
            this._theSchemasComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSchemasComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theSchemasComboBox.FormattingEnabled = true;
            this._theSchemasComboBox.IntegralHeight = false;
            this._theSchemasComboBox.Location = new System.Drawing.Point(74, 4);
            this._theSchemasComboBox.Name = "_theSchemasComboBox";
            this._theSchemasComboBox.SelectedTrafodionObject = null;
            this._theSchemasComboBox.SelectedTrafodionSchema = null;
            this._theSchemasComboBox.Size = new System.Drawing.Size(570, 21);
            this._theSchemasComboBox.Sorted = true;
            this._theSchemasComboBox.TabIndex = 1;
            this._theSchemasComboBox.TheTrafodionCatalog = null;
            this._theSchemasComboBox.SelectedIndexChanged += new System.EventHandler(this.TheSchemasComboBoxSelectedIndexChanged);
            // 
            // _theSystemsPanel
            // 
            this._theSystemsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSystemsPanel.Controls.Add(this._theMySystemsComboBox);
            this._theSystemsPanel.Controls.Add(label1);
            this._theSystemsPanel.Dock = System.Windows.Forms.DockStyle.Left;
            this._theSystemsPanel.Location = new System.Drawing.Point(0, 0);
            this._theSystemsPanel.Name = "_theSystemsPanel";
            this._theSystemsPanel.Size = new System.Drawing.Size(210, 29);
            this._theSystemsPanel.TabIndex = 1;
            // 
            // _theMySystemsComboBox
            // 
            this._theMySystemsComboBox.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this._theMySystemsComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theMySystemsComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theMySystemsComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theMySystemsComboBox.FormattingEnabled = true;
            this._theMySystemsComboBox.IsLoading = false;
            this._theMySystemsComboBox.Location = new System.Drawing.Point(66, 5);
            this._theMySystemsComboBox.Name = "_theMySystemsComboBox";
            this._theMySystemsComboBox.SelectedCatalogName = null;
            this._theMySystemsComboBox.SelectedConnectionDefinition = null;
            this._theMySystemsComboBox.SelectedSchemaName = null;
            this._theMySystemsComboBox.Size = new System.Drawing.Size(140, 21);
            this._theMySystemsComboBox.Sorted = true;
            this._theMySystemsComboBox.TabIndex = 0;
            this._theMySystemsComboBox.SelectedIndexChanged += new System.EventHandler(this.TheMySystemsComboBoxSelectedIndexChanged);
            // 
            // headerPanel
            // 
            this.headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.headerPanel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.headerPanel.Location = new System.Drawing.Point(0, 0);
            this.headerPanel.Name = "headerPanel";
            this.headerPanel.Size = new System.Drawing.Size(857, 26);
            this.headerPanel.TabIndex = 15;
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionToolStrip1.Dock = System.Windows.Forms.DockStyle.None;
            this.TrafodionToolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theHelpStripButton});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(542, 2);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(102, 25);
            this.TrafodionToolStrip1.TabIndex = 0;
            this.TrafodionToolStrip1.Text = "Help";
            this.TrafodionToolStrip1.Visible = false;
            // 
            // _theHelpStripButton
            // 
            this._theHelpStripButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theHelpStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theHelpStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theHelpStripButton.Image")));
            this._theHelpStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theHelpStripButton.Name = "_theHelpStripButton";
            this._theHelpStripButton.Size = new System.Drawing.Size(23, 22);
            this._theHelpStripButton.Text = "Help";
            this._theHelpStripButton.Visible = false;
            // 
            // theRowCountsPanel
            // 
            this.theRowCountsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theRowCountsPanel.Controls.Add(this.theRowsPerPagePanel);
            this.theRowCountsPanel.Controls.Add(this._theMaxRowsPanel);
            this.theRowCountsPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theRowCountsPanel.Location = new System.Drawing.Point(0, 84);
            this.theRowCountsPanel.Name = "theRowCountsPanel";
            this.theRowCountsPanel.Size = new System.Drawing.Size(857, 28);
            this.theRowCountsPanel.TabIndex = 16;
            // 
            // theRowsPerPagePanel
            // 
            this.theRowsPerPagePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theRowsPerPagePanel.Controls.Add(this._includeTableStatsCheckBox);
            this.theRowsPerPagePanel.Controls.Add(this.TrafodionToolStrip1);
            this.theRowsPerPagePanel.Controls.Add(rowsPerPageLabel);
            this.theRowsPerPagePanel.Controls.Add(this._rowsPerPageTextBox);
            this.theRowsPerPagePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theRowsPerPagePanel.Location = new System.Drawing.Point(210, 0);
            this.theRowsPerPagePanel.Name = "theRowsPerPagePanel";
            this.theRowsPerPagePanel.Size = new System.Drawing.Size(647, 28);
            this.theRowsPerPagePanel.TabIndex = 2;
            // 
            // _includeTableStatsCheckBox
            // 
            this._includeTableStatsCheckBox.AutoSize = true;
            this._includeTableStatsCheckBox.Checked = true;
            this._includeTableStatsCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._includeTableStatsCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._includeTableStatsCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._includeTableStatsCheckBox.Location = new System.Drawing.Point(227, 6);
            this._includeTableStatsCheckBox.Name = "_includeTableStatsCheckBox";
            this._includeTableStatsCheckBox.Size = new System.Drawing.Size(198, 18);
            this._includeTableStatsCheckBox.TabIndex = 2;
            this._includeTableStatsCheckBox.Text = "Fetch Table Statistics After Explain";
            this._includeTableStatsCheckBox.UseVisualStyleBackColor = true;
            // 
            // _rowsPerPageTextBox
            // 
            this._rowsPerPageTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._rowsPerPageTextBox.Location = new System.Drawing.Point(74, 4);
            this._rowsPerPageTextBox.Name = "_rowsPerPageTextBox";
            this._rowsPerPageTextBox.Size = new System.Drawing.Size(140, 21);
            this._rowsPerPageTextBox.TabIndex = 1;
            this._rowsPerPageTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this._rowsPerPageTextBox_KeyPress);
            this._rowsPerPageTextBox.KeyUp += new System.Windows.Forms.KeyEventHandler(this._rowsPerPageTextBox_KeyUp);
            // 
            // TrafodionToolTip1
            // 
            this.TrafodionToolTip1.IsBalloon = true;
            // 
            // _theStatementSplitContainer
            // 
            this._theStatementSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theStatementSplitContainer.Location = new System.Drawing.Point(0, 112);
            this._theStatementSplitContainer.Name = "_theStatementSplitContainer";
            this._theStatementSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _theStatementSplitContainer.Panel1
            // 
            this._theStatementSplitContainer.Panel1.Controls.Add(this.TrafodionPanel1);
            this._theStatementSplitContainer.Panel1MinSize = 60;
            // 
            // _theStatementSplitContainer.Panel2
            // 
            this._theStatementSplitContainer.Panel2.Controls.Add(this.TrafodionGroupBox1);
            this._theStatementSplitContainer.Panel2MinSize = 90;
            this._theStatementSplitContainer.Size = new System.Drawing.Size(857, 238);
            this._theStatementSplitContainer.SplitterDistance = 103;
            this._theStatementSplitContainer.SplitterWidth = 9;
            this._theStatementSplitContainer.TabIndex = 17;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._theQueryInputControl);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(857, 103);
            this.TrafodionPanel1.TabIndex = 15;
            // 
            // _theQueryInputControl
            // 
            this._theQueryInputControl.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._theQueryInputControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryInputControl.Location = new System.Drawing.Point(0, 0);
            this._theQueryInputControl.Name = "_theQueryInputControl";
            this._theQueryInputControl.Size = new System.Drawing.Size(857, 103);
            this._theQueryInputControl.TabIndex = 14;
            this._theQueryInputControl.TheConnectionDefinition = null;
            this._theQueryInputControl.TheMaxRows = 500;
            this._theQueryInputControl.TheReportDefinition = null;
            this._theQueryInputControl.TheStatement = "";
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._theCQSettingControl);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(857, 126);
            this.TrafodionGroupBox1.TabIndex = 1;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Control Settings";
            // 
            // _theCQSettingControl
            // 
            this._theCQSettingControl.ConnectionDefn = null;
            this._theCQSettingControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theCQSettingControl.Location = new System.Drawing.Point(3, 17);
            this._theCQSettingControl.Name = "_theCQSettingControl";
            this._theCQSettingControl.Size = new System.Drawing.Size(851, 106);
            this._theCQSettingControl.TabIndex = 0;
            // 
            // QueryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theStatementSplitContainer);
            this.Controls.Add(this.theRowCountsPanel);
            this.Controls.Add(this._theSystemsAndSchemasPanel);
            this.Controls.Add(_theNameCatalogsAndMaxRowsPanel);
            this.Controls.Add(this.headerPanel);
            this.Controls.Add(panel2);
            this.Name = "QueryUserControl";
            this.Size = new System.Drawing.Size(857, 380);
            panel2.ResumeLayout(false);
            _theNameCatalogsAndMaxRowsPanel.ResumeLayout(false);
            this._theCatalogsPanel.ResumeLayout(false);
            this._theCatalogsPanel.PerformLayout();
            this._thaNamePanel.ResumeLayout(false);
            this._thaNamePanel.PerformLayout();
            this._theMaxRowsPanel.ResumeLayout(false);
            this._theMaxRowsPanel.PerformLayout();
            this._theSystemsAndSchemasPanel.ResumeLayout(false);
            this._theSchemasPanel.ResumeLayout(false);
            this._theSchemasPanel.PerformLayout();
            this._theSystemsPanel.ResumeLayout(false);
            this._theSystemsPanel.PerformLayout();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.theRowCountsPanel.ResumeLayout(false);
            this.theRowsPerPagePanel.ResumeLayout(false);
            this.theRowsPerPagePanel.PerformLayout();
            this._theStatementSplitContainer.Panel1.ResumeLayout(false);
            this._theStatementSplitContainer.Panel2.ResumeLayout(false);
            this._theStatementSplitContainer.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionButton _theAddButton;
        private TrafodionButton _theExplainButton;
        private TrafodionButton _theExecuteButton;
        private TrafodionButton _theUpdateButton;
        private QueryInputControl _theQueryInputControl;
        private TrafodionPanel _theSystemsAndSchemasPanel;
        private Trafodion.Manager.DatabaseArea.Controls.SchemasComboBox _theSchemasComboBox;
        private TrafodionTextBox _theNameTextBox;
        private TrafodionPanel _theMaxRowsPanel;
        private TrafodionTextBox _theMaxRowsTextBox;
        private TrafodionPanel _theCatalogsPanel;
        private Trafodion.Manager.DatabaseArea.Controls.CatalogsComboBox _theCatalogsComboBox;
        private Trafodion.Manager.Framework.Connections.Controls.MySystemsComboBox _theMySystemsComboBox;
        private TrafodionPanel _theSchemasPanel;
        private TrafodionPanel _theSystemsPanel;
        private TrafodionPanel _thaNamePanel;
        private TrafodionPanel headerPanel;
        private TrafodionPanel theRowCountsPanel;
        private TrafodionTextBox _rowsPerPageTextBox;
        private TrafodionPanel theRowsPerPagePanel;
        private TrafodionToolTip TrafodionToolTip1;
        private TrafodionSplitContainer _theStatementSplitContainer;
        private CQSettingControl _theCQSettingControl;
        private TrafodionGroupBox TrafodionGroupBox1;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _theHelpStripButton;
        private TrafodionCheckBox _includeTableStatsCheckBox;
    }
}
