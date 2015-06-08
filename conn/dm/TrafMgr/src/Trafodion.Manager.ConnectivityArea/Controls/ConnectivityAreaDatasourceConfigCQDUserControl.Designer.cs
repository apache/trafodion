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
    partial class ConnectivityAreaDatasourceConfigCQDUserControl
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
            this.cqdAdd_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.cqdModify_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cqdRemove_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.tableLayoutPanel2.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 35);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(832, 578);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // cqdAdd_TrafodionButton
            // 
            this.cqdAdd_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.cqdAdd_TrafodionButton.Location = new System.Drawing.Point(3, 3);
            this.cqdAdd_TrafodionButton.Name = "cqdAdd_TrafodionButton";
            this.cqdAdd_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.cqdAdd_TrafodionButton.TabIndex = 0;
            this.cqdAdd_TrafodionButton.Text = "Add...";
            this.cqdAdd_TrafodionButton.UseVisualStyleBackColor = true;
            this.cqdAdd_TrafodionButton.Click += new System.EventHandler(this.cqdAdd_TrafodionButton_Click);
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.ColumnCount = 1;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Controls.Add(this.flowLayoutPanel2, 0, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 1;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(832, 34);
            this.tableLayoutPanel2.TabIndex = 8;
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.flowLayoutPanel2.Controls.Add(this.cqdAdd_TrafodionButton);
            this.flowLayoutPanel2.Controls.Add(this.cqdModify_TrafodionButton);
            this.flowLayoutPanel2.Controls.Add(this.cqdRemove_TrafodionButton);
            this.flowLayoutPanel2.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(294, 3);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(244, 28);
            this.flowLayoutPanel2.TabIndex = 3;
            // 
            // cqdModify_TrafodionButton
            // 
            this.cqdModify_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.cqdModify_TrafodionButton.Location = new System.Drawing.Point(84, 3);
            this.cqdModify_TrafodionButton.Name = "cqdModify_TrafodionButton";
            this.cqdModify_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.cqdModify_TrafodionButton.TabIndex = 1;
            this.cqdModify_TrafodionButton.Text = "Modify...";
            this.cqdModify_TrafodionButton.UseVisualStyleBackColor = true;
            this.cqdModify_TrafodionButton.Click += new System.EventHandler(this.cqdModify_TrafodionButton_Click);
            // 
            // cqdRemove_TrafodionButton
            // 
            this.cqdRemove_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.cqdRemove_TrafodionButton.Location = new System.Drawing.Point(165, 3);
            this.cqdRemove_TrafodionButton.Name = "cqdRemove_TrafodionButton";
            this.cqdRemove_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.cqdRemove_TrafodionButton.TabIndex = 2;
            this.cqdRemove_TrafodionButton.Text = "Remove";
            this.cqdRemove_TrafodionButton.UseVisualStyleBackColor = true;
            this.cqdRemove_TrafodionButton.Click += new System.EventHandler(this.cqdRemove_TrafodionButton_Click);
            // 
            // ConnectivityAreaDatasourceConfigCQDUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tableLayoutPanel2);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaDatasourceConfigCQDUserControl";
            this.Size = new System.Drawing.Size(832, 613);
            this.tableLayoutPanel2.ResumeLayout(false);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion


        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cqdAdd_TrafodionButton;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cqdModify_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cqdRemove_TrafodionButton;

    }
}
