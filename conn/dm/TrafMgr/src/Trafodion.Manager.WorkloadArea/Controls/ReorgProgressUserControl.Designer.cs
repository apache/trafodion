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
    partial class ReorgProgressUserControl
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
            MyDispose(disposing);
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ReorgProgressUserControl));
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theQueryIdTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._thePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.infoIGridGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this._statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._progressBar = new System.Windows.Forms.ToolStripProgressBar();
            this.infoIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this._summartGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._partitionStatusGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._reorgErrorPtText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgProgressPtText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgCompletedPtText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotNeededPtLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgRunningPtText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._totalPartitionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotStartedPtText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgErrorPtLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotStartedPtLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotNeededPtText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgRunningPtLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._totalPartitionsText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgProgressPtLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgCompletedPtLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._tableStatusGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._reorgErrorText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgProgressText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgCompletedText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgRunningText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotStartedText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotNeededText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._totalTablesText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgErrorLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgProgressLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgCompletedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgRunningLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotStartedLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._reorgNotNeededLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._totalTablesLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theStopQuery = new System.Windows.Forms.ToolStripButton();
            this._theRefreshButton = new System.Windows.Forms.ToolStripButton();
            this._theHelpButton = new System.Windows.Forms.ToolStripButton();
            this.panel1.SuspendLayout();
            this._thePanel.SuspendLayout();
            this.infoIGridGroupBox.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.infoIGrid)).BeginInit();
            this._summartGroupBox.SuspendLayout();
            this._partitionStatusGroupBox.SuspendLayout();
            this._tableStatusGroupBox.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this._theQueryIdTextBox);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(827, 54);
            this.panel1.TabIndex = 3;
            // 
            // _theQueryIdTextBox
            // 
            this._theQueryIdTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theQueryIdTextBox.Location = new System.Drawing.Point(62, 17);
            this._theQueryIdTextBox.Name = "_theQueryIdTextBox";
            this._theQueryIdTextBox.ReadOnly = true;
            this._theQueryIdTextBox.Size = new System.Drawing.Size(760, 21);
            this._theQueryIdTextBox.TabIndex = 1;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label1.Location = new System.Drawing.Point(4, 21);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(55, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Query ID:";
            // 
            // _thePanel
            // 
            this._thePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._thePanel.Controls.Add(this.infoIGridGroupBox);
            this._thePanel.Controls.Add(this._summartGroupBox);
            this._thePanel.Controls.Add(this.TrafodionToolStrip1);
            this._thePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePanel.Location = new System.Drawing.Point(0, 54);
            this._thePanel.Name = "_thePanel";
            this._thePanel.Size = new System.Drawing.Size(827, 435);
            this._thePanel.TabIndex = 4;
            // 
            // infoIGridGroupBox
            // 
            this.infoIGridGroupBox.Controls.Add(this.statusStrip1);
            this.infoIGridGroupBox.Controls.Add(this.infoIGrid);
            this.infoIGridGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.infoIGridGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.infoIGridGroupBox.Location = new System.Drawing.Point(0, 218);
            this.infoIGridGroupBox.Name = "infoIGridGroupBox";
            this.infoIGridGroupBox.Size = new System.Drawing.Size(827, 217);
            this.infoIGridGroupBox.TabIndex = 5;
            this.infoIGridGroupBox.TabStop = false;
            this.infoIGridGroupBox.Text = "Reorg Progress Details";
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._statusLabel,
            this._progressBar});
            this.statusStrip1.Location = new System.Drawing.Point(3, 192);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(821, 22);
            this.statusStrip1.TabIndex = 1;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // _statusLabel
            // 
            this._statusLabel.Name = "_statusLabel";
            this._statusLabel.Size = new System.Drawing.Size(71, 17);
            this._statusLabel.Text = "_statusLabel";
            // 
            // _progressBar
            // 
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(160, 16);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // infoIGrid
            // 
            this.infoIGrid.AllowColumnFilter = true;
            this.infoIGrid.AllowWordWrap = false;
            this.infoIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("infoIGrid.AlwaysHiddenColumnNames")));
            this.infoIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.infoIGrid.CurrentFilter = null;
            this.infoIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.infoIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.infoIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.infoIGrid.Header.Height = 19;
            this.infoIGrid.HelpTopic = "";
            this.infoIGrid.Location = new System.Drawing.Point(3, 17);
            this.infoIGrid.Name = "infoIGrid";
            this.infoIGrid.ReadOnly = true;
            this.infoIGrid.RowMode = true;
            this.infoIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.infoIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.infoIGrid.SearchAsType.SearchCol = null;
            this.infoIGrid.Size = new System.Drawing.Size(821, 197);
            this.infoIGrid.TabIndex = 0;
            this.infoIGrid.TreeCol = null;
            this.infoIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.infoIGrid.WordWrap = false;
            // 
            // _summartGroupBox
            // 
            this._summartGroupBox.Controls.Add(this._partitionStatusGroupBox);
            this._summartGroupBox.Controls.Add(this._tableStatusGroupBox);
            this._summartGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._summartGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._summartGroupBox.Location = new System.Drawing.Point(0, 25);
            this._summartGroupBox.Name = "_summartGroupBox";
            this._summartGroupBox.Padding = new System.Windows.Forms.Padding(10);
            this._summartGroupBox.Size = new System.Drawing.Size(827, 193);
            this._summartGroupBox.TabIndex = 4;
            this._summartGroupBox.TabStop = false;
            this._summartGroupBox.Text = "Reorg Progress Summary";
            // 
            // _partitionStatusGroupBox
            // 
            this._partitionStatusGroupBox.AutoSize = true;
            this._partitionStatusGroupBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._partitionStatusGroupBox.Controls.Add(this._reorgErrorPtText);
            this._partitionStatusGroupBox.Controls.Add(this._reorgProgressPtText);
            this._partitionStatusGroupBox.Controls.Add(this._reorgCompletedPtText);
            this._partitionStatusGroupBox.Controls.Add(this._reorgNotNeededPtLabel);
            this._partitionStatusGroupBox.Controls.Add(this._reorgRunningPtText);
            this._partitionStatusGroupBox.Controls.Add(this._totalPartitionLabel);
            this._partitionStatusGroupBox.Controls.Add(this._reorgNotStartedPtText);
            this._partitionStatusGroupBox.Controls.Add(this._reorgErrorPtLabel);
            this._partitionStatusGroupBox.Controls.Add(this._reorgNotStartedPtLabel);
            this._partitionStatusGroupBox.Controls.Add(this._reorgNotNeededPtText);
            this._partitionStatusGroupBox.Controls.Add(this._reorgRunningPtLabel);
            this._partitionStatusGroupBox.Controls.Add(this._totalPartitionsText);
            this._partitionStatusGroupBox.Controls.Add(this._reorgProgressPtLabel);
            this._partitionStatusGroupBox.Controls.Add(this._reorgCompletedPtLabel);
            this._partitionStatusGroupBox.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._partitionStatusGroupBox.Location = new System.Drawing.Point(407, 17);
            this._partitionStatusGroupBox.Name = "_partitionStatusGroupBox";
            this._partitionStatusGroupBox.Size = new System.Drawing.Size(412, 171);
            this._partitionStatusGroupBox.TabIndex = 3;
            this._partitionStatusGroupBox.TabStop = false;
            this._partitionStatusGroupBox.Text = "Partition";
            // 
            // _reorgErrorPtText
            // 
            this._reorgErrorPtText.AutoSize = true;
            this._reorgErrorPtText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgErrorPtText.Location = new System.Drawing.Point(146, 118);
            this._reorgErrorPtText.Name = "_reorgErrorPtText";
            this._reorgErrorPtText.Size = new System.Drawing.Size(32, 16);
            this._reorgErrorPtText.TabIndex = 1;
            this._reorgErrorPtText.Text = "100";
            // 
            // _reorgProgressPtText
            // 
            this._reorgProgressPtText.AutoSize = true;
            this._reorgProgressPtText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgProgressPtText.Location = new System.Drawing.Point(146, 137);
            this._reorgProgressPtText.Name = "_reorgProgressPtText";
            this._reorgProgressPtText.Size = new System.Drawing.Size(32, 16);
            this._reorgProgressPtText.TabIndex = 1;
            this._reorgProgressPtText.Text = "100";
            // 
            // _reorgCompletedPtText
            // 
            this._reorgCompletedPtText.AutoSize = true;
            this._reorgCompletedPtText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgCompletedPtText.Location = new System.Drawing.Point(146, 99);
            this._reorgCompletedPtText.Name = "_reorgCompletedPtText";
            this._reorgCompletedPtText.Size = new System.Drawing.Size(32, 16);
            this._reorgCompletedPtText.TabIndex = 1;
            this._reorgCompletedPtText.Text = "100";
            // 
            // _reorgNotNeededPtLabel
            // 
            this._reorgNotNeededPtLabel.AutoSize = true;
            this._reorgNotNeededPtLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotNeededPtLabel.Location = new System.Drawing.Point(25, 40);
            this._reorgNotNeededPtLabel.Name = "_reorgNotNeededPtLabel";
            this._reorgNotNeededPtLabel.Size = new System.Drawing.Size(121, 16);
            this._reorgNotNeededPtLabel.TabIndex = 0;
            this._reorgNotNeededPtLabel.Text = "Reorg Not Needed :";
            // 
            // _reorgRunningPtText
            // 
            this._reorgRunningPtText.AutoSize = true;
            this._reorgRunningPtText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgRunningPtText.Location = new System.Drawing.Point(146, 80);
            this._reorgRunningPtText.Name = "_reorgRunningPtText";
            this._reorgRunningPtText.Size = new System.Drawing.Size(32, 16);
            this._reorgRunningPtText.TabIndex = 1;
            this._reorgRunningPtText.Text = "100";
            // 
            // _totalPartitionLabel
            // 
            this._totalPartitionLabel.AutoSize = true;
            this._totalPartitionLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._totalPartitionLabel.Location = new System.Drawing.Point(44, 23);
            this._totalPartitionLabel.Name = "_totalPartitionLabel";
            this._totalPartitionLabel.Size = new System.Drawing.Size(103, 16);
            this._totalPartitionLabel.TabIndex = 0;
            this._totalPartitionLabel.Text = "Total Partitions :";
            // 
            // _reorgNotStartedPtText
            // 
            this._reorgNotStartedPtText.AutoSize = true;
            this._reorgNotStartedPtText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotStartedPtText.Location = new System.Drawing.Point(146, 61);
            this._reorgNotStartedPtText.Name = "_reorgNotStartedPtText";
            this._reorgNotStartedPtText.Size = new System.Drawing.Size(32, 16);
            this._reorgNotStartedPtText.TabIndex = 1;
            this._reorgNotStartedPtText.Text = "100";
            // 
            // _reorgErrorPtLabel
            // 
            this._reorgErrorPtLabel.AutoSize = true;
            this._reorgErrorPtLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgErrorPtLabel.Location = new System.Drawing.Point(63, 118);
            this._reorgErrorPtLabel.Name = "_reorgErrorPtLabel";
            this._reorgErrorPtLabel.Size = new System.Drawing.Size(84, 16);
            this._reorgErrorPtLabel.TabIndex = 0;
            this._reorgErrorPtLabel.Text = "Reorg Error :";
            // 
            // _reorgNotStartedPtLabel
            // 
            this._reorgNotStartedPtLabel.AutoSize = true;
            this._reorgNotStartedPtLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotStartedPtLabel.Location = new System.Drawing.Point(27, 61);
            this._reorgNotStartedPtLabel.Name = "_reorgNotStartedPtLabel";
            this._reorgNotStartedPtLabel.Size = new System.Drawing.Size(120, 16);
            this._reorgNotStartedPtLabel.TabIndex = 0;
            this._reorgNotStartedPtLabel.Text = "Reorg Not Started :";
            // 
            // _reorgNotNeededPtText
            // 
            this._reorgNotNeededPtText.AutoSize = true;
            this._reorgNotNeededPtText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotNeededPtText.Location = new System.Drawing.Point(146, 41);
            this._reorgNotNeededPtText.Name = "_reorgNotNeededPtText";
            this._reorgNotNeededPtText.Size = new System.Drawing.Size(32, 16);
            this._reorgNotNeededPtText.TabIndex = 1;
            this._reorgNotNeededPtText.Text = "100";
            // 
            // _reorgRunningPtLabel
            // 
            this._reorgRunningPtLabel.AutoSize = true;
            this._reorgRunningPtLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgRunningPtLabel.Location = new System.Drawing.Point(46, 79);
            this._reorgRunningPtLabel.Name = "_reorgRunningPtLabel";
            this._reorgRunningPtLabel.Size = new System.Drawing.Size(101, 16);
            this._reorgRunningPtLabel.TabIndex = 0;
            this._reorgRunningPtLabel.Text = "Reorg Running :";
            // 
            // _totalPartitionsText
            // 
            this._totalPartitionsText.AutoSize = true;
            this._totalPartitionsText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._totalPartitionsText.Location = new System.Drawing.Point(146, 23);
            this._totalPartitionsText.Name = "_totalPartitionsText";
            this._totalPartitionsText.Size = new System.Drawing.Size(32, 16);
            this._totalPartitionsText.TabIndex = 1;
            this._totalPartitionsText.Text = "100";
            // 
            // _reorgProgressPtLabel
            // 
            this._reorgProgressPtLabel.AutoSize = true;
            this._reorgProgressPtLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgProgressPtLabel.Location = new System.Drawing.Point(26, 137);
            this._reorgProgressPtLabel.Name = "_reorgProgressPtLabel";
            this._reorgProgressPtLabel.Size = new System.Drawing.Size(121, 16);
            this._reorgProgressPtLabel.TabIndex = 0;
            this._reorgProgressPtLabel.Text = "Reorg Progress % :";
            // 
            // _reorgCompletedPtLabel
            // 
            this._reorgCompletedPtLabel.AutoSize = true;
            this._reorgCompletedPtLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgCompletedPtLabel.Location = new System.Drawing.Point(31, 99);
            this._reorgCompletedPtLabel.Name = "_reorgCompletedPtLabel";
            this._reorgCompletedPtLabel.Size = new System.Drawing.Size(116, 16);
            this._reorgCompletedPtLabel.TabIndex = 0;
            this._reorgCompletedPtLabel.Text = "Reorg Completed :";
            // 
            // _tableStatusGroupBox
            // 
            this._tableStatusGroupBox.AutoSize = true;
            this._tableStatusGroupBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._tableStatusGroupBox.Controls.Add(this._reorgErrorText);
            this._tableStatusGroupBox.Controls.Add(this._reorgProgressText);
            this._tableStatusGroupBox.Controls.Add(this._reorgCompletedText);
            this._tableStatusGroupBox.Controls.Add(this._reorgRunningText);
            this._tableStatusGroupBox.Controls.Add(this._reorgNotStartedText);
            this._tableStatusGroupBox.Controls.Add(this._reorgNotNeededText);
            this._tableStatusGroupBox.Controls.Add(this._totalTablesText);
            this._tableStatusGroupBox.Controls.Add(this._reorgErrorLabel);
            this._tableStatusGroupBox.Controls.Add(this._reorgProgressLabel);
            this._tableStatusGroupBox.Controls.Add(this._reorgCompletedLabel);
            this._tableStatusGroupBox.Controls.Add(this._reorgRunningLabel);
            this._tableStatusGroupBox.Controls.Add(this._reorgNotStartedLabel);
            this._tableStatusGroupBox.Controls.Add(this._reorgNotNeededLabel);
            this._tableStatusGroupBox.Controls.Add(this._totalTablesLabel);
            this._tableStatusGroupBox.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._tableStatusGroupBox.Location = new System.Drawing.Point(13, 17);
            this._tableStatusGroupBox.Name = "_tableStatusGroupBox";
            this._tableStatusGroupBox.Size = new System.Drawing.Size(376, 172);
            this._tableStatusGroupBox.TabIndex = 2;
            this._tableStatusGroupBox.TabStop = false;
            this._tableStatusGroupBox.Text = "Table";
            // 
            // _reorgErrorText
            // 
            this._reorgErrorText.AutoSize = true;
            this._reorgErrorText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgErrorText.Location = new System.Drawing.Point(148, 118);
            this._reorgErrorText.Name = "_reorgErrorText";
            this._reorgErrorText.Size = new System.Drawing.Size(32, 16);
            this._reorgErrorText.TabIndex = 1;
            this._reorgErrorText.Text = "100";
            // 
            // _reorgProgressText
            // 
            this._reorgProgressText.AutoSize = true;
            this._reorgProgressText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgProgressText.Location = new System.Drawing.Point(148, 138);
            this._reorgProgressText.Name = "_reorgProgressText";
            this._reorgProgressText.Size = new System.Drawing.Size(32, 16);
            this._reorgProgressText.TabIndex = 1;
            this._reorgProgressText.Text = "100";
            // 
            // _reorgCompletedText
            // 
            this._reorgCompletedText.AutoSize = true;
            this._reorgCompletedText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgCompletedText.Location = new System.Drawing.Point(148, 99);
            this._reorgCompletedText.Name = "_reorgCompletedText";
            this._reorgCompletedText.Size = new System.Drawing.Size(32, 16);
            this._reorgCompletedText.TabIndex = 1;
            this._reorgCompletedText.Text = "100";
            // 
            // _reorgRunningText
            // 
            this._reorgRunningText.AutoSize = true;
            this._reorgRunningText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgRunningText.Location = new System.Drawing.Point(148, 79);
            this._reorgRunningText.Name = "_reorgRunningText";
            this._reorgRunningText.Size = new System.Drawing.Size(32, 16);
            this._reorgRunningText.TabIndex = 1;
            this._reorgRunningText.Text = "100";
            // 
            // _reorgNotStartedText
            // 
            this._reorgNotStartedText.AutoSize = true;
            this._reorgNotStartedText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotStartedText.Location = new System.Drawing.Point(148, 61);
            this._reorgNotStartedText.Name = "_reorgNotStartedText";
            this._reorgNotStartedText.Size = new System.Drawing.Size(32, 16);
            this._reorgNotStartedText.TabIndex = 1;
            this._reorgNotStartedText.Text = "100";
            // 
            // _reorgNotNeededText
            // 
            this._reorgNotNeededText.AutoSize = true;
            this._reorgNotNeededText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotNeededText.Location = new System.Drawing.Point(148, 41);
            this._reorgNotNeededText.Name = "_reorgNotNeededText";
            this._reorgNotNeededText.Size = new System.Drawing.Size(32, 16);
            this._reorgNotNeededText.TabIndex = 1;
            this._reorgNotNeededText.Text = "100";
            // 
            // _totalTablesText
            // 
            this._totalTablesText.AutoSize = true;
            this._totalTablesText.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._totalTablesText.Location = new System.Drawing.Point(148, 23);
            this._totalTablesText.Name = "_totalTablesText";
            this._totalTablesText.Size = new System.Drawing.Size(32, 16);
            this._totalTablesText.TabIndex = 1;
            this._totalTablesText.Text = "100";
            // 
            // _reorgErrorLabel
            // 
            this._reorgErrorLabel.AutoSize = true;
            this._reorgErrorLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgErrorLabel.Location = new System.Drawing.Point(62, 117);
            this._reorgErrorLabel.Name = "_reorgErrorLabel";
            this._reorgErrorLabel.Size = new System.Drawing.Size(84, 16);
            this._reorgErrorLabel.TabIndex = 0;
            this._reorgErrorLabel.Text = "Reorg Error :";
            // 
            // _reorgProgressLabel
            // 
            this._reorgProgressLabel.AutoSize = true;
            this._reorgProgressLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgProgressLabel.Location = new System.Drawing.Point(24, 137);
            this._reorgProgressLabel.Name = "_reorgProgressLabel";
            this._reorgProgressLabel.Size = new System.Drawing.Size(121, 16);
            this._reorgProgressLabel.TabIndex = 0;
            this._reorgProgressLabel.Text = "Reorg Progress % :";
            // 
            // _reorgCompletedLabel
            // 
            this._reorgCompletedLabel.AutoSize = true;
            this._reorgCompletedLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgCompletedLabel.Location = new System.Drawing.Point(30, 99);
            this._reorgCompletedLabel.Name = "_reorgCompletedLabel";
            this._reorgCompletedLabel.Size = new System.Drawing.Size(116, 16);
            this._reorgCompletedLabel.TabIndex = 0;
            this._reorgCompletedLabel.Text = "Reorg Completed :";
            // 
            // _reorgRunningLabel
            // 
            this._reorgRunningLabel.AutoSize = true;
            this._reorgRunningLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgRunningLabel.Location = new System.Drawing.Point(45, 79);
            this._reorgRunningLabel.Name = "_reorgRunningLabel";
            this._reorgRunningLabel.Size = new System.Drawing.Size(101, 16);
            this._reorgRunningLabel.TabIndex = 0;
            this._reorgRunningLabel.Text = "Reorg Running :";
            // 
            // _reorgNotStartedLabel
            // 
            this._reorgNotStartedLabel.AutoSize = true;
            this._reorgNotStartedLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotStartedLabel.Location = new System.Drawing.Point(26, 60);
            this._reorgNotStartedLabel.Name = "_reorgNotStartedLabel";
            this._reorgNotStartedLabel.Size = new System.Drawing.Size(120, 16);
            this._reorgNotStartedLabel.TabIndex = 0;
            this._reorgNotStartedLabel.Text = "Reorg Not Started :";
            // 
            // _reorgNotNeededLabel
            // 
            this._reorgNotNeededLabel.AutoSize = true;
            this._reorgNotNeededLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._reorgNotNeededLabel.Location = new System.Drawing.Point(24, 40);
            this._reorgNotNeededLabel.Name = "_reorgNotNeededLabel";
            this._reorgNotNeededLabel.Size = new System.Drawing.Size(121, 16);
            this._reorgNotNeededLabel.TabIndex = 0;
            this._reorgNotNeededLabel.Text = "Reorg Not Needed :";
            // 
            // _totalTablesLabel
            // 
            this._totalTablesLabel.AutoSize = true;
            this._totalTablesLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._totalTablesLabel.Location = new System.Drawing.Point(62, 21);
            this._totalTablesLabel.Name = "_totalTablesLabel";
            this._totalTablesLabel.Size = new System.Drawing.Size(84, 16);
            this._totalTablesLabel.TabIndex = 0;
            this._totalTablesLabel.Text = "Total tables :";
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theStopQuery,
            this._theRefreshButton,
            this._theHelpButton});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(827, 25);
            this.TrafodionToolStrip1.TabIndex = 3;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // _theStopQuery
            // 
            this._theStopQuery.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theStopQuery.Enabled = false;
            this._theStopQuery.Image = ((System.Drawing.Image)(resources.GetObject("_theStopQuery.Image")));
            this._theStopQuery.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theStopQuery.Name = "_theStopQuery";
            this._theStopQuery.Size = new System.Drawing.Size(23, 22);
            this._theStopQuery.Text = "Stop Fetching data";
            this._theStopQuery.ToolTipText = "Stop Fetching data";
            this._theStopQuery.Click += new System.EventHandler(this._theStopQuery_Click);
            // 
            // _theRefreshButton
            // 
            this._theRefreshButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRefreshButton.Image")));
            this._theRefreshButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRefreshButton.Name = "_theRefreshButton";
            this._theRefreshButton.Size = new System.Drawing.Size(23, 22);
            this._theRefreshButton.Text = "Refresh Reorg Progress";
            this._theRefreshButton.ToolTipText = "Refresh SQL Text";
            this._theRefreshButton.Click += new System.EventHandler(this._theRefreshButton_Click);
            // 
            // _theHelpButton
            // 
            this._theHelpButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theHelpButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theHelpButton.Image = ((System.Drawing.Image)(resources.GetObject("_theHelpButton.Image")));
            this._theHelpButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theHelpButton.Name = "_theHelpButton";
            this._theHelpButton.Size = new System.Drawing.Size(23, 22);
            this._theHelpButton.Text = "_theHelpButton";
            this._theHelpButton.ToolTipText = "Help";
            this._theHelpButton.Click += new System.EventHandler(this._theHelpButton_Click);
            // 
            // ReorgProgressUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._thePanel);
            this.Controls.Add(this.panel1);
            this.Name = "ReorgProgressUserControl";
            this.Size = new System.Drawing.Size(827, 489);
            this.Resize += new System.EventHandler(this.ReorgProgressUserControl_Resize);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this._thePanel.ResumeLayout(false);
            this._thePanel.PerformLayout();
            this.infoIGridGroupBox.ResumeLayout(false);
            this.infoIGridGroupBox.PerformLayout();
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.infoIGrid)).EndInit();
            this._summartGroupBox.ResumeLayout(false);
            this._summartGroupBox.PerformLayout();
            this._partitionStatusGroupBox.ResumeLayout(false);
            this._partitionStatusGroupBox.PerformLayout();
            this._tableStatusGroupBox.ResumeLayout(false);
            this._tableStatusGroupBox.PerformLayout();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theQueryIdTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _thePanel;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _theRefreshButton;
        private System.Windows.Forms.ToolStripButton _theHelpButton;
        private Framework.Controls.TrafodionGroupBox _summartGroupBox;
        private Framework.Controls.TrafodionGroupBox _tableStatusGroupBox;
        private Framework.Controls.TrafodionLabel _reorgCompletedText;
        private Framework.Controls.TrafodionLabel _reorgRunningText;
        private Framework.Controls.TrafodionLabel _reorgNotStartedText;
        private Framework.Controls.TrafodionLabel _reorgNotNeededText;
        private Framework.Controls.TrafodionLabel _totalTablesText;
        private Framework.Controls.TrafodionLabel _reorgCompletedLabel;
        private Framework.Controls.TrafodionLabel _reorgRunningLabel;
        private Framework.Controls.TrafodionLabel _reorgNotStartedLabel;
        private Framework.Controls.TrafodionLabel _reorgNotNeededLabel;
        private Framework.Controls.TrafodionLabel _totalTablesLabel;
        private Framework.Controls.TrafodionGroupBox _partitionStatusGroupBox;
        private Framework.Controls.TrafodionIGrid _reorgProgressDetailsIGrid;
        private Framework.Controls.TrafodionLabel _reorgCompletedPtText;
        private Framework.Controls.TrafodionLabel _reorgNotNeededPtLabel;
        private Framework.Controls.TrafodionLabel _reorgRunningPtText;
        private Framework.Controls.TrafodionLabel _totalPartitionLabel;
        private Framework.Controls.TrafodionLabel _reorgNotStartedPtText;
        private Framework.Controls.TrafodionLabel _reorgNotStartedPtLabel;
        private Framework.Controls.TrafodionLabel _reorgNotNeededPtText;
        private Framework.Controls.TrafodionLabel _reorgRunningPtLabel;
        private Framework.Controls.TrafodionLabel _totalPartitionsText;
        private Framework.Controls.TrafodionLabel _reorgCompletedPtLabel;
        private Framework.Controls.TrafodionGroupBox infoIGridGroupBox;
        private Framework.Controls.TrafodionIGrid infoIGrid;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripProgressBar _progressBar;
        private System.Windows.Forms.ToolStripStatusLabel _statusLabel;
        private Framework.Controls.TrafodionLabel _reorgProgressPtText;
        private Framework.Controls.TrafodionLabel _reorgProgressPtLabel;
        private Framework.Controls.TrafodionLabel _reorgProgressText;
        private Framework.Controls.TrafodionLabel _reorgProgressLabel;
        private System.Windows.Forms.ToolStripButton _theStopQuery;
        private Framework.Controls.TrafodionLabel _reorgErrorPtText;
        private Framework.Controls.TrafodionLabel _reorgErrorPtLabel;
        private Framework.Controls.TrafodionLabel _reorgErrorText;
        private Framework.Controls.TrafodionLabel _reorgErrorLabel;
    }
}
