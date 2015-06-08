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
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class TimeSelector
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
            this.panel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.minusHour = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.addHour = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.addDay = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.minusDay = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.panel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.timeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.timeComboToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.panel1.SuspendLayout();
            this.panel3.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this.panel3);
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(209, 53);
            this.panel1.TabIndex = 0;
            // 
            // panel3
            // 
            this.panel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel3.Controls.Add(this.flowLayoutPanel2);
            this.panel3.Controls.Add(this.flowLayoutPanel1);
            this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel3.Location = new System.Drawing.Point(0, 24);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(209, 29);
            this.panel3.TabIndex = 1;
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.flowLayoutPanel2.Controls.Add(this.minusHour);
            this.flowLayoutPanel2.Controls.Add(this.addHour);
            this.flowLayoutPanel2.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(103, 0);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(106, 29);
            this.flowLayoutPanel2.TabIndex = 1;
            // 
            // minusHour
            // 
            this.minusHour.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.minusHour.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.minusHour.Location = new System.Drawing.Point(58, 3);
            this.minusHour.Name = "minusHour";
            this.minusHour.Size = new System.Drawing.Size(45, 23);
            this.minusHour.TabIndex = 0;
            this.minusHour.Text = "Hour -";
            this.minusHour.UseVisualStyleBackColor = true;
            this.minusHour.Click += new System.EventHandler(this.minusHour_Click);
            // 
            // addHour
            // 
            this.addHour.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.addHour.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.addHour.Location = new System.Drawing.Point(3, 3);
            this.addHour.Name = "addHour";
            this.addHour.Size = new System.Drawing.Size(49, 23);
            this.addHour.TabIndex = 1;
            this.addHour.Text = "Hour +";
            this.addHour.UseVisualStyleBackColor = true;
            this.addHour.Click += new System.EventHandler(this.addHour_Click);
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.flowLayoutPanel1.Controls.Add(this.addDay);
            this.flowLayoutPanel1.Controls.Add(this.minusDay);
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(104, 29);
            this.flowLayoutPanel1.TabIndex = 0;
            // 
            // addDay
            // 
            this.addDay.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.addDay.Location = new System.Drawing.Point(3, 3);
            this.addDay.Name = "addDay";
            this.addDay.Size = new System.Drawing.Size(45, 23);
            this.addDay.TabIndex = 0;
            this.addDay.Text = "Day +";
            this.addDay.UseVisualStyleBackColor = true;
            this.addDay.Click += new System.EventHandler(this.addDay_Click);
            // 
            // minusDay
            // 
            this.minusDay.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.minusDay.Location = new System.Drawing.Point(54, 3);
            this.minusDay.Name = "minusDay";
            this.minusDay.Size = new System.Drawing.Size(41, 23);
            this.minusDay.TabIndex = 1;
            this.minusDay.Text = "Day - ";
            this.minusDay.UseVisualStyleBackColor = true;
            this.minusDay.Click += new System.EventHandler(this.minusDay_Click);
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel2.Controls.Add(this.timeCombo);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel2.Location = new System.Drawing.Point(0, 0);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(209, 22);
            this.panel2.TabIndex = 0;
            // 
            // timeCombo
            // 
            this.timeCombo.Dock = System.Windows.Forms.DockStyle.Top;
            this.timeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.timeCombo.Font = new System.Drawing.Font("Tahoma", 8F);
            this.timeCombo.FormattingEnabled = true;
            this.timeCombo.Location = new System.Drawing.Point(0, 0);
            this.timeCombo.Name = "timeCombo";
            this.timeCombo.Size = new System.Drawing.Size(209, 21);
            this.timeCombo.TabIndex = 0;
            this.timeCombo.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.timeCombo_KeyPress);
            this.timeCombo.TextChanged += new System.EventHandler(this.timeCombo_TextChanged);
            // 
            // timeComboToolTip
            // 
            this.timeComboToolTip.IsBalloon = true;
            // 
            // TimeSelector
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.panel1);
            this.Name = "TimeSelector";
            this.Size = new System.Drawing.Size(209, 53);
            this.panel1.ResumeLayout(false);
            this.panel3.ResumeLayout(false);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionPanel panel1;
        private TrafodionPanel panel2;
        private TrafodionComboBox timeCombo;
        private TrafodionPanel panel3;
        private FlowLayoutPanel flowLayoutPanel2;
        private TrafodionButton minusHour;
        private TrafodionButton addHour;
        private FlowLayoutPanel flowLayoutPanel1;
        private TrafodionButton addDay;
        private TrafodionButton minusDay;
        private TrafodionToolTip timeComboToolTip;
    }
}
