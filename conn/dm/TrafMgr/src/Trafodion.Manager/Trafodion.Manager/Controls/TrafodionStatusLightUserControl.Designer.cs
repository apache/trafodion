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
﻿namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionStatusLightUserControl
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
            this.components = new System.ComponentModel.Container();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.statusLight_pictureBox = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.lightLabel_label = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.CSelectedIndicator = new Trafodion.Manager.Framework.Controls.TrafodionTabGraphic();
            this.panel6 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.statusLight_pictureBox)).BeginInit();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this.statusLight_pictureBox);
            this.panel1.Controls.Add(this.tableLayoutPanel1);
            this.panel1.Controls.Add(this.CSelectedIndicator);
            this.panel1.Controls.Add(this.panel6);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Margin = new System.Windows.Forms.Padding(0, 3, 0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(168, 169);
            this.panel1.TabIndex = 1;
            // 
            // statusLight_pictureBox
            // 
            this.statusLight_pictureBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this.statusLight_pictureBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.statusLight_pictureBox.Image = global::Trafodion.Manager.Properties.Resources.GrayIcon;
            this.statusLight_pictureBox.Location = new System.Drawing.Point(0, 0);
            this.statusLight_pictureBox.Name = "statusLight_pictureBox";
            this.statusLight_pictureBox.Size = new System.Drawing.Size(168, 149);
            this.statusLight_pictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.statusLight_pictureBox.TabIndex = 2;
            this.statusLight_pictureBox.TabStop = false;
            this.statusLight_pictureBox.Click += new System.EventHandler(this.statusLight_pictureBox_Click);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.lightLabel_label, 0, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 149);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 15F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(168, 15);
            this.tableLayoutPanel1.TabIndex = 15;
            // 
            // lightLabel_label
            // 
            this.lightLabel_label.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.lightLabel_label.AutoEllipsis = true;
            this.lightLabel_label.AutoSize = true;
            this.lightLabel_label.BackColor = System.Drawing.Color.WhiteSmoke;
            this.lightLabel_label.Font = new System.Drawing.Font("Verdana", 8F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lightLabel_label.Location = new System.Drawing.Point(40, 1);
            this.lightLabel_label.Name = "lightLabel_label";
            this.lightLabel_label.Size = new System.Drawing.Size(88, 13);
            this.lightLabel_label.TabIndex = 3;
            this.lightLabel_label.Text = "Connectivity";
            this.lightLabel_label.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // CSelectedIndicator
            // 
            this.CSelectedIndicator.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.CSelectedIndicator.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CSelectedIndicator.Location = new System.Drawing.Point(0, 0);
            this.CSelectedIndicator.Name = "CSelectedIndicator";
            this.CSelectedIndicator.Size = new System.Drawing.Size(168, 169);
            this.CSelectedIndicator.TabIndex = 14;
            this.CSelectedIndicator.Visible = false;
            // 
            // panel6
            // 
            this.panel6.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel6.BackgroundImage = global::Trafodion.Manager.Properties.Resources.TopTop2;
            this.panel6.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel6.Location = new System.Drawing.Point(0, 164);
            this.panel6.Name = "panel6";
            this.panel6.Size = new System.Drawing.Size(168, 5);
            this.panel6.TabIndex = 6;
            this.panel6.Visible = false;
            // 
            // toolTip1
            // 
            this.toolTip1.IsBalloon = true;
            // 
            // TrafodionStatusLightUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.panel1);
            this.Name = "TrafodionStatusLightUserControl";
            this.Size = new System.Drawing.Size(168, 169);
            this.panel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.statusLight_pictureBox)).EndInit();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPictureBox statusLight_pictureBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel lightLabel_label;
        private TrafodionTabGraphic CSelectedIndicator;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel6;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
    }
}
