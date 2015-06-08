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
﻿namespace BDRPlugin11.Controls
{
    partial class BDRDetail_STALE
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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle4 = new System.Windows.Forms.DataGridViewCellStyle();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(BDRDetail_STALE));
            this._merge10GroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._merge10DataGridView = new Trafodion.Manager.Framework.Controls.TrafodionDataGridView();
            this._partitionsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._partitionsDataGridView = new Trafodion.Manager.Framework.Controls.TrafodionDataGridView();
            this._widgetCanvas3 = new Trafodion.Manager.Framework.WidgetCanvas();
            this._merge10GroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._merge10DataGridView)).BeginInit();
            this._partitionsGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._partitionsDataGridView)).BeginInit();
            this.SuspendLayout();
            // 
            // _merge10GroupBox
            // 
            this._merge10GroupBox.Controls.Add(this._merge10DataGridView);
            this._merge10GroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._merge10GroupBox.Location = new System.Drawing.Point(33, 93);
            this._merge10GroupBox.Name = "_merge10GroupBox";
            this._merge10GroupBox.Size = new System.Drawing.Size(288, 260);
            this._merge10GroupBox.TabIndex = 0;
            this._merge10GroupBox.TabStop = false;
            this._merge10GroupBox.Text = "Group=10";
            // 
            // _merge10DataGridView
            // 
            this._merge10DataGridView.AllowUserToAddRows = false;
            this._merge10DataGridView.AllowUserToOrderColumns = true;
            dataGridViewCellStyle1.BackColor = System.Drawing.Color.WhiteSmoke;
            this._merge10DataGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle1;
            this._merge10DataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this._merge10DataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            this._merge10DataGridView.BackgroundColor = System.Drawing.Color.WhiteSmoke;
            this._merge10DataGridView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._merge10DataGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this._merge10DataGridView.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle2;
            this._merge10DataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this._merge10DataGridView.Dock = System.Windows.Forms.DockStyle.Fill;
            this._merge10DataGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this._merge10DataGridView.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._merge10DataGridView.GridColor = System.Drawing.Color.LightGray;
            this._merge10DataGridView.Location = new System.Drawing.Point(3, 17);
            this._merge10DataGridView.Name = "_merge10DataGridView";
            this._merge10DataGridView.RowHeadersVisible = false;
            this._merge10DataGridView.RowTemplate.ReadOnly = true;
            this._merge10DataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this._merge10DataGridView.Size = new System.Drawing.Size(282, 240);
            this._merge10DataGridView.TabIndex = 0;
            this._merge10DataGridView.TheHeaderText = "";
            // 
            // _partitionsGroupBox
            // 
            this._partitionsGroupBox.Controls.Add(this._partitionsDataGridView);
            this._partitionsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._partitionsGroupBox.Location = new System.Drawing.Point(340, 24);
            this._partitionsGroupBox.Name = "_partitionsGroupBox";
            this._partitionsGroupBox.Size = new System.Drawing.Size(276, 260);
            this._partitionsGroupBox.TabIndex = 1;
            this._partitionsGroupBox.TabStop = false;
            this._partitionsGroupBox.Text = "Active Partitions";
            // 
            // _partitionsDataGridView
            // 
            this._partitionsDataGridView.AllowUserToAddRows = false;
            this._partitionsDataGridView.AllowUserToOrderColumns = true;
            dataGridViewCellStyle3.BackColor = System.Drawing.Color.WhiteSmoke;
            this._partitionsDataGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle3;
            this._partitionsDataGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.DisplayedCellsExceptHeader;
            this._partitionsDataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            this._partitionsDataGridView.BackgroundColor = System.Drawing.Color.WhiteSmoke;
            this._partitionsDataGridView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._partitionsDataGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            dataGridViewCellStyle4.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle4.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle4.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            dataGridViewCellStyle4.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle4.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle4.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle4.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this._partitionsDataGridView.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle4;
            this._partitionsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this._partitionsDataGridView.Dock = System.Windows.Forms.DockStyle.Fill;
            this._partitionsDataGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this._partitionsDataGridView.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._partitionsDataGridView.GridColor = System.Drawing.Color.LightGray;
            this._partitionsDataGridView.Location = new System.Drawing.Point(3, 17);
            this._partitionsDataGridView.Name = "_partitionsDataGridView";
            this._partitionsDataGridView.RowHeadersVisible = false;
            this._partitionsDataGridView.RowTemplate.ReadOnly = true;
            this._partitionsDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this._partitionsDataGridView.Size = new System.Drawing.Size(270, 240);
            this._partitionsDataGridView.TabIndex = 0;
            this._partitionsDataGridView.TheHeaderText = "";
            // 
            // _widgetCanvas3
            // 
            this._widgetCanvas3.ActiveWidget = null;
            this._widgetCanvas3.AllowDelete = true;
            this._widgetCanvas3.AllowDrop = true;
            this._widgetCanvas3.LayoutManager = null;
            this._widgetCanvas3.Location = new System.Drawing.Point(361, 261);
            this._widgetCanvas3.Name = "_widgetCanvas3";
            this._widgetCanvas3.Size = new System.Drawing.Size(388, 119);
            this._widgetCanvas3.TabIndex = 2;
            this._widgetCanvas3.ThePersistenceKey = null;
            this._widgetCanvas3.ViewName = null;
            this._widgetCanvas3.ViewNum = 0;
            this._widgetCanvas3.ViewText = null;
            this._widgetCanvas3.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("_widgetCanvas3.WidgetsModel")));
            // 
            // MyBDRDetail
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(761, 589);
            this.Controls.Add(this._widgetCanvas3);
            this.Controls.Add(this._partitionsGroupBox);
            this.Controls.Add(this._merge10GroupBox);
            this.Name = "MyBDRDetail";
            this.Text = "MyBDRDetail";
            this.Shown += new System.EventHandler(this.MyBDRDetail_Shown);
            this._merge10GroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._merge10DataGridView)).EndInit();
            this._partitionsGroupBox.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this._partitionsDataGridView)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _merge10GroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionDataGridView _merge10DataGridView;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _partitionsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionDataGridView _partitionsDataGridView;
        private Trafodion.Manager.Framework.WidgetCanvas _widgetCanvas3;
    }
}