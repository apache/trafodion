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
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivitySystemSummaryUserControl
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
            this._summaryPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._connectivitysummarylistView = new System.Windows.Forms.ListView();
            this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
            this.columnHeader2 = new System.Windows.Forms.ColumnHeader();
            this._buttonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._Summarylabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._summaryPanel.SuspendLayout();
            this._buttonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _summaryPanel
            // 
            this._summaryPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._summaryPanel.Controls.Add(this._Summarylabel1);
            this._summaryPanel.Controls.Add(this._connectivitysummarylistView);
            this._summaryPanel.Controls.Add(this._buttonPanel);
            this._summaryPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._summaryPanel.Location = new System.Drawing.Point(0, 0);
            this._summaryPanel.Name = "_summaryPanel";
            this._summaryPanel.Size = new System.Drawing.Size(566, 418);
            this._summaryPanel.TabIndex = 0;
            // 
            // _connectivitysummarylistView
            // 
            this._connectivitysummarylistView.AutoArrange = false;
            this._connectivitysummarylistView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2});
            this._connectivitysummarylistView.Cursor = System.Windows.Forms.Cursors.Default;
            this._connectivitysummarylistView.FullRowSelect = true;
            this._connectivitysummarylistView.GridLines = true;
            this._connectivitysummarylistView.Location = new System.Drawing.Point(4, 3);
            this._connectivitysummarylistView.MultiSelect = false;
            this._connectivitysummarylistView.Name = "_connectivitysummarylistView";
            this._connectivitysummarylistView.Scrollable = false;
            this._connectivitysummarylistView.Size = new System.Drawing.Size(383, 123);
            this._connectivitysummarylistView.TabIndex = 0;
            this._connectivitysummarylistView.TabStop = false;
            this._connectivitysummarylistView.UseCompatibleStateImageBehavior = false;
            this._connectivitysummarylistView.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "Connectivity Property";
            this.columnHeader1.Width = 267;
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = "Curreny Value";
            this.columnHeader2.Width = 160;
            // 
            // _buttonPanel
            // 
            this._buttonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._buttonPanel.Controls.Add(this._refreshButton);
            this._buttonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonPanel.Location = new System.Drawing.Point(0, 385);
            this._buttonPanel.Name = "_buttonPanel";
            this._buttonPanel.Size = new System.Drawing.Size(566, 33);
            this._buttonPanel.TabIndex = 6;
            // 
            // _refreshButton
            // 
            this._refreshButton.Location = new System.Drawing.Point(3, 5);
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(79, 23);
            this._refreshButton.TabIndex = 0;
            this._refreshButton.Text = "&Refresh";
            this._refreshButton.UseVisualStyleBackColor = true;
            this._refreshButton.Click += new System.EventHandler(this._refreshButton_Click);
            // 
            // _Summarylabel1
            // 
            this._Summarylabel1.AutoSize = true;
            this._Summarylabel1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._Summarylabel1.Location = new System.Drawing.Point(3, 129);
            this._Summarylabel1.Name = "_Summarylabel1";
            this._Summarylabel1.Size = new System.Drawing.Size(308, 13);
            this._Summarylabel1.TabIndex = 7;
            this._Summarylabel1.Text = "Summary Last Updated At: 03-15-2009 14:39:33 PDT";
            // 
            // ConnectivitySystemSummaryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.Controls.Add(this._summaryPanel);
            this.Name = "ConnectivitySystemSummaryUserControl";
            this.Size = new System.Drawing.Size(566, 418);
            this._summaryPanel.ResumeLayout(false);
            this._summaryPanel.PerformLayout();
            this._buttonPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _summaryPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _buttonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _refreshButton;
        private System.Windows.Forms.ListView _connectivitysummarylistView;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _Summarylabel1;
    }
}
