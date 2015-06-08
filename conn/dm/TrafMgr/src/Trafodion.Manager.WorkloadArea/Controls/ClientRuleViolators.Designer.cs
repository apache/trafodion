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
    partial class ClientRuleViolators
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ClientRuleViolators));
            TenTec.Windows.iGridLib.iGColPattern iGColPattern1 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern2 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern3 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern4 = new TenTec.Windows.iGridLib.iGColPattern();
            TenTec.Windows.iGridLib.iGColPattern iGColPattern5 = new TenTec.Windows.iGridLib.iGColPattern();
            this.iGridRuleViolatorsCol2CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGridRuleViolatorsCol2ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGridRuleViolatorsCol3CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGridRuleViolatorsCol3ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGridRuleViolatorsCol4CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGridRuleViolatorsCol4ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGridRuleViolators = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.iGCellStyleDesign1 = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.treeView_Systems = new System.Windows.Forms.TreeView();
            this.iGColHdrStyleDesign1 = new TenTec.Windows.iGridLib.iGColHdrStyleDesign();
            ((System.ComponentModel.ISupportInitialize)(this.iGridRuleViolators)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // iGridRuleViolators
            // 
            this.iGridRuleViolators.AllowColumnFilter = true;
            this.iGridRuleViolators.AllowWordWrap = false;
            this.iGridRuleViolators.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("iGridRuleViolators.AlwaysHiddenColumnNames")));
            this.iGridRuleViolators.AutoResizeCols = true;
            this.iGridRuleViolators.AutoWidthColMode = TenTec.Windows.iGridLib.iGAutoWidthColMode.Cells;
            this.iGridRuleViolators.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            iGColPattern1.AllowMoving = false;
            iGColPattern1.AllowSizing = false;
            iGColPattern1.ColHdrStyle = this.iGColHdrStyleDesign1;
            iGColPattern1.IncludeInSelect = false;
            iGColPattern1.Key = "RuleName";
            iGColPattern1.Text = "Rule Name";
            iGColPattern1.Visible = false;
            iGColPattern2.CellStyle = this.iGridRuleViolatorsCol2CellStyle;
            iGColPattern2.ColHdrStyle = this.iGridRuleViolatorsCol2ColHdrStyle;
            iGColPattern2.Key = "SystemName";
            iGColPattern2.Text = "System Name";
            iGColPattern2.Width = 72;
            iGColPattern3.Key = "QueryID";
            iGColPattern3.Text = "Query ID";
            iGColPattern3.Width = 121;
            iGColPattern4.CellStyle = this.iGridRuleViolatorsCol3CellStyle;
            iGColPattern4.ColHdrStyle = this.iGridRuleViolatorsCol3ColHdrStyle;
            iGColPattern4.Key = "WorkloadMonitor";
            iGColPattern4.Text = "Workload Monitor";
            iGColPattern4.Width = 71;
            iGColPattern5.CellStyle = this.iGridRuleViolatorsCol4CellStyle;
            iGColPattern5.ColHdrStyle = this.iGridRuleViolatorsCol4ColHdrStyle;
            iGColPattern5.Key = "StartTime";
            iGColPattern5.Text = "Start Time";
            iGColPattern5.Width = 71;
            this.iGridRuleViolators.Cols.AddRange(new TenTec.Windows.iGridLib.iGColPattern[] {
            iGColPattern1,
            iGColPattern2,
            iGColPattern3,
            iGColPattern4,
            iGColPattern5});
            this.iGridRuleViolators.CurrentFilter = null;
            this.iGridRuleViolators.Dock = System.Windows.Forms.DockStyle.Fill;
            this.iGridRuleViolators.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.iGridRuleViolators.ForeColor = System.Drawing.Color.MidnightBlue;
            this.iGridRuleViolators.GroupRowLevelStyles = new TenTec.Windows.iGridLib.iGCellStyle[] {
        ((TenTec.Windows.iGridLib.iGCellStyle)(this.iGCellStyleDesign1))};
            this.iGridRuleViolators.Header.Height = 20;
            this.iGridRuleViolators.HelpTopic = "";
            this.iGridRuleViolators.Location = new System.Drawing.Point(0, 0);
            this.iGridRuleViolators.Name = "iGridRuleViolators";
            this.iGridRuleViolators.ReadOnly = true;
            this.iGridRuleViolators.RowMode = true;
            this.iGridRuleViolators.RowModeHasCurCell = true;
            this.iGridRuleViolators.RowTextVisible = true;
            this.iGridRuleViolators.ScrollGroupRows = true;
            this.iGridRuleViolators.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.iGridRuleViolators.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.iGridRuleViolators.SearchAsType.SearchCol = null;
            this.iGridRuleViolators.Size = new System.Drawing.Size(339, 374);
            this.iGridRuleViolators.SortByLevels = true;
            this.iGridRuleViolators.TabIndex = 2;
            this.iGridRuleViolators.TreeCol = null;
            this.iGridRuleViolators.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.iGridRuleViolators.WordWrap = false;
            this.iGridRuleViolators.SelectionChanged += new System.EventHandler(this.iGridRuleViolators_SelectionChanged);
            // 
            // iGCellStyleDesign1
            // 
            this.iGCellStyleDesign1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.iGCellStyleDesign1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.treeView_Systems);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.iGridRuleViolators);
            this.splitContainer1.Size = new System.Drawing.Size(592, 374);
            this.splitContainer1.SplitterDistance = 249;
            this.splitContainer1.TabIndex = 3;
            // 
            // treeView_Systems
            // 
            this.treeView_Systems.CheckBoxes = true;
            this.treeView_Systems.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeView_Systems.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.treeView_Systems.Location = new System.Drawing.Point(0, 0);
            this.treeView_Systems.Name = "treeView_Systems";
            this.treeView_Systems.Size = new System.Drawing.Size(249, 374);
            this.treeView_Systems.TabIndex = 2;
            // 
            // ClientRuleViolators
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(592, 374);
            this.Controls.Add(this.splitContainer1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.Name = "ClientRuleViolators";
            this.ShowIcon = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "Trafodion Database Manager - Threshold Rule Violations";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.ClientRuleViolators_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.iGridRuleViolators)).EndInit();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TenTec.Windows.iGridLib.iGCellStyleDesign iGCellStyleDesign1;
        private TenTec.Windows.iGridLib.iGCellStyle iGridRuleViolatorsCol2CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGridRuleViolatorsCol2ColHdrStyle;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.TreeView treeView_Systems;
        private TenTec.Windows.iGridLib.iGCellStyle iGridRuleViolatorsCol3CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGridRuleViolatorsCol3ColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGridRuleViolatorsCol4CellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGridRuleViolatorsCol4ColHdrStyle;
        private Framework.Controls.TrafodionIGrid iGridRuleViolators;
        private TenTec.Windows.iGridLib.iGColHdrStyleDesign iGColHdrStyleDesign1;
    }
}