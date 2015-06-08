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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class HealthPopupDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(HealthPopupDialog));
            this.TrafodionIGridSubjectArea = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._helpButton = new System.Windows.Forms.ToolStripButton();
            this.TrafodionLabelLayer = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theCurrentTimestampLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            ((System.ComponentModel.ISupportInitialize)(this.TrafodionIGridSubjectArea)).BeginInit();
            this.TrafodionToolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionIGridSubjectArea
            // 
            this.TrafodionIGridSubjectArea.AllowColumnFilter = true;
            this.TrafodionIGridSubjectArea.AllowWordWrap = false;
            this.TrafodionIGridSubjectArea.AlwaysHiddenColumnNames = ((System.Collections.Generic.List<string>)(resources.GetObject("TrafodionIGridSubjectArea.AlwaysHiddenColumnNames")));
            this.TrafodionIGridSubjectArea.AutoResizeCols = true;
            this.TrafodionIGridSubjectArea.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.TrafodionIGridSubjectArea.CurrentFilter = null;
            this.TrafodionIGridSubjectArea.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionIGridSubjectArea.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionIGridSubjectArea.ForeColor = System.Drawing.SystemColors.WindowText;
            this.TrafodionIGridSubjectArea.Header.Height = 20;
            this.TrafodionIGridSubjectArea.HelpTopic = "";
            this.TrafodionIGridSubjectArea.Location = new System.Drawing.Point(0, 48);
            this.TrafodionIGridSubjectArea.Name = "TrafodionIGridSubjectArea";
            this.TrafodionIGridSubjectArea.ReadOnly = true;
            this.TrafodionIGridSubjectArea.RowMode = true;
            this.TrafodionIGridSubjectArea.SearchAsType.MatchRule = TenTec.Windows.iGridLib.iGMatchRule.Contains;
            this.TrafodionIGridSubjectArea.SearchAsType.Mode = TenTec.Windows.iGridLib.iGSearchAsTypeMode.Seek;
            this.TrafodionIGridSubjectArea.SearchAsType.SearchCol = null;
            this.TrafodionIGridSubjectArea.Size = new System.Drawing.Size(565, 220);
            this.TrafodionIGridSubjectArea.TabIndex = 0;
            this.TrafodionIGridSubjectArea.TreeCol = null;
            this.TrafodionIGridSubjectArea.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this.TrafodionIGridSubjectArea.WordWrap = false;
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Dock = System.Windows.Forms.DockStyle.Right;
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._helpButton});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(541, 0);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(24, 48);
            this.TrafodionToolStrip1.TabIndex = 1;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // _helpButton
            // 
            this._helpButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._helpButton.Image = ((System.Drawing.Image)(resources.GetObject("_helpButton.Image")));
            this._helpButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(21, 20);
            this._helpButton.Text = "Help";
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // TrafodionLabelLayer
            // 
            this.TrafodionLabelLayer.AutoSize = true;
            this.TrafodionLabelLayer.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TrafodionLabelLayer.Location = new System.Drawing.Point(12, 11);
            this.TrafodionLabelLayer.Name = "TrafodionLabelLayer";
            this.TrafodionLabelLayer.Size = new System.Drawing.Size(45, 16);
            this.TrafodionLabelLayer.TabIndex = 0;
            this.TrafodionLabelLayer.Text = "Layer";
            // 
            // _theCurrentTimestampLabel
            // 
            this._theCurrentTimestampLabel.AutoSize = true;
            this._theCurrentTimestampLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theCurrentTimestampLabel.Location = new System.Drawing.Point(245, 11);
            this._theCurrentTimestampLabel.Name = "_theCurrentTimestampLabel";
            this._theCurrentTimestampLabel.Size = new System.Drawing.Size(0, 16);
            this._theCurrentTimestampLabel.TabIndex = 2;
            // 
            // HealthPopupDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(565, 268);
            this.Controls.Add(this._theCurrentTimestampLabel);
            this.Controls.Add(this.TrafodionToolStrip1);
            this.Controls.Add(this.TrafodionLabelLayer);
            this.Controls.Add(this.TrafodionIGridSubjectArea);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "HealthPopupDialog";
            this.Text = "HP Database Manager - HealthPopupDialog";
            ((System.ComponentModel.ISupportInitialize)(this.TrafodionIGridSubjectArea)).EndInit();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionIGrid TrafodionIGridSubjectArea;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _helpButton;
        private Framework.Controls.TrafodionLabel TrafodionLabelLayer;
        private Framework.Controls.TrafodionLabel _theCurrentTimestampLabel;
    }
}