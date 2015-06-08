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
    partial class WMSHistory
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSHistory));
            this.historyIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0AT" +
                    "QB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAI" +
                    "ABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this.iGrid1DefaultCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.iGrid1DefaultColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this.iGrid1RowTextColCellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this.okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.clearButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.groupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            ((System.ComponentModel.ISupportInitialize)(this.historyIGrid)).BeginInit();
            this.panel1.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // historyIGrid
            // 
            this.historyIGrid.AllowColumnFilter = true;
            this.historyIGrid.AllowWordWrap = false;
            this.historyIGrid.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("historyIGrid.AlwaysHiddenColumnNames")));
            this.historyIGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.historyIGrid.CurrentFilter = null;
            this.historyIGrid.DefaultCol.CellStyle = this.iGrid1DefaultCellStyle;
            this.historyIGrid.DefaultCol.ColHdrStyle = this.iGrid1DefaultColHdrStyle;
            this.historyIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.historyIGrid.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.historyIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this.historyIGrid.Header.Height = 19;
            this.historyIGrid.HelpTopic = "";
            this.historyIGrid.Location = new System.Drawing.Point(3, 17);
            this.historyIGrid.Name = "historyIGrid";
            this.historyIGrid.ReadOnly = true;
            this.historyIGrid.RowMode = true;
            this.historyIGrid.RowTextCol.CellStyle = this.iGrid1RowTextColCellStyle;
            this.historyIGrid.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.historyIGrid.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.historyIGrid.SearchAsType.SearchCol = null;
            this.historyIGrid.Size = new System.Drawing.Size(698, 304);
            this.historyIGrid.TabIndex = 0;
            this.historyIGrid.TreeCol = null;
            this.historyIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.historyIGrid.WordWrap = false;
            // 
            // okButton
            // 
            this.okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.okButton.Enabled = false;
            this.okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.okButton.Location = new System.Drawing.Point(461, 2);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(75, 23);
            this.okButton.TabIndex = 1;
            this.okButton.Text = "OK";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.buttonOK_Click);
            // 
            // refreshButton
            // 
            this.refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshButton.Location = new System.Drawing.Point(84, 2);
            this.refreshButton.Name = "refreshButton";
            this.refreshButton.Size = new System.Drawing.Size(75, 23);
            this.refreshButton.TabIndex = 3;
            this.refreshButton.Text = "&Refresh";
            this.refreshButton.UseVisualStyleBackColor = true;
            this.refreshButton.Click += new System.EventHandler(this.buttonRefresh_Click);
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this.clearButton);
            this.panel1.Controls.Add(this.helpButton);
            this.panel1.Controls.Add(this.cancelButton);
            this.panel1.Controls.Add(this.okButton);
            this.panel1.Controls.Add(this.refreshButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 324);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(704, 28);
            this.panel1.TabIndex = 4;
            // 
            // clearButton
            // 
            this.clearButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.clearButton.Location = new System.Drawing.Point(3, 2);
            this.clearButton.Name = "clearButton";
            this.clearButton.Size = new System.Drawing.Size(75, 23);
            this.clearButton.TabIndex = 2;
            this.clearButton.Text = "C&lear";
            this.clearButton.UseVisualStyleBackColor = true;
            this.clearButton.Click += new System.EventHandler(this.clearButton_Click);
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(623, 2);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 23);
            this.helpButton.TabIndex = 1;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(542, 2);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 1;
            this.cancelButton.Text = "&Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.historyIGrid);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox1.Location = new System.Drawing.Point(0, 0);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(704, 324);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Offender Commands";
            // 
            // WMSHistory
            // 
            this.AcceptButton = this.okButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(704, 352);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.panel1);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "WMSHistory";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Offender Command History";
            ((System.ComponentModel.ISupportInitialize)(this.historyIGrid)).EndInit();
            this.panel1.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionIGrid historyIGrid;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1DefaultCellStyle;
        private TenTec.Windows.iGridLib.iGColHdrStyle iGrid1DefaultColHdrStyle;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid1RowTextColCellStyle;
        private Trafodion.Manager.Framework.Controls.TrafodionButton okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton refreshButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton clearButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton helpButton;
    }
}