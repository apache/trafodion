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
﻿namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaDatasourceConfigControlledTablesUserControl
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
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.ctModifyControlledTable_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.addControlledTable_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.ctRemoveControlledTable_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 34);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(703, 548);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // ctModifyControlledTable_TrafodionButton
            // 
            this.ctModifyControlledTable_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.ctModifyControlledTable_TrafodionButton.Location = new System.Drawing.Point(84, 3);
            this.ctModifyControlledTable_TrafodionButton.Name = "ctModifyControlledTable_TrafodionButton";
            this.ctModifyControlledTable_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.ctModifyControlledTable_TrafodionButton.TabIndex = 1;
            this.ctModifyControlledTable_TrafodionButton.Text = "Modify...";
            this.ctModifyControlledTable_TrafodionButton.UseVisualStyleBackColor = true;
            this.ctModifyControlledTable_TrafodionButton.Click += new System.EventHandler(this.ctModifyControlledTable_TrafodionButton_Click);
            // 
            // addControlledTable_TrafodionButton
            // 
            this.addControlledTable_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.addControlledTable_TrafodionButton.Location = new System.Drawing.Point(3, 3);
            this.addControlledTable_TrafodionButton.Name = "addControlledTable_TrafodionButton";
            this.addControlledTable_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.addControlledTable_TrafodionButton.TabIndex = 0;
            this.addControlledTable_TrafodionButton.Text = "Add...";
            this.addControlledTable_TrafodionButton.UseVisualStyleBackColor = true;
            this.addControlledTable_TrafodionButton.Click += new System.EventHandler(this.addControlledTable_TrafodionButton_Click);
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.flowLayoutPanel1.Controls.Add(this.addControlledTable_TrafodionButton);
            this.flowLayoutPanel1.Controls.Add(this.ctModifyControlledTable_TrafodionButton);
            this.flowLayoutPanel1.Controls.Add(this.ctRemoveControlledTable_TrafodionButton);
            this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(229, 3);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(244, 28);
            this.flowLayoutPanel1.TabIndex = 3;
            // 
            // ctRemoveControlledTable_TrafodionButton
            // 
            this.ctRemoveControlledTable_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.ctRemoveControlledTable_TrafodionButton.Location = new System.Drawing.Point(165, 3);
            this.ctRemoveControlledTable_TrafodionButton.Name = "ctRemoveControlledTable_TrafodionButton";
            this.ctRemoveControlledTable_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.ctRemoveControlledTable_TrafodionButton.TabIndex = 2;
            this.ctRemoveControlledTable_TrafodionButton.Text = "Remove";
            this.ctRemoveControlledTable_TrafodionButton.UseVisualStyleBackColor = true;
            this.ctRemoveControlledTable_TrafodionButton.Click += new System.EventHandler(this.ctRemoveControlledTable_TrafodionButton_Click);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel1, 0, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(703, 34);
            this.tableLayoutPanel1.TabIndex = 4;
            // 
            // ConnectivityAreaDatasourceConfigControlledTablesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tableLayoutPanel1);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaDatasourceConfigControlledTablesUserControl";
            this.Size = new System.Drawing.Size(703, 582);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton ctModifyControlledTable_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton addControlledTable_TrafodionButton;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton ctRemoveControlledTable_TrafodionButton;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;

    }
}
