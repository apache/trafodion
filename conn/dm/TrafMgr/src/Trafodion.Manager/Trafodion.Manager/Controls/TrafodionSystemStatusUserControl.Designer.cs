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
    partial class TrafodionSystemStatusUserControl
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
                MyDispose(disposing);
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
            this.LightClickHandler = new TrafodionStatusLightUserControl.ChangingHandler(MouseClickLight);
            this.components = new System.ComponentModel.Container();
            this.statusLightTable_tableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.socketClientBindingSource = new System.Windows.Forms.BindingSource(this.components);
            this.socketClientBindingSource1 = new System.Windows.Forms.BindingSource(this.components);
            this.errorTableDataBindingSource = new System.Windows.Forms.BindingSource(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.socketClientBindingSource)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.socketClientBindingSource1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.errorTableDataBindingSource)).BeginInit();
            this.SuspendLayout();
            // 
            // statusLightTable_tableLayoutPanel
            // 
            this.statusLightTable_tableLayoutPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.statusLightTable_tableLayoutPanel.ColumnCount = 1;
            this.statusLightTable_tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.statusLightTable_tableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.statusLightTable_tableLayoutPanel.GrowStyle = System.Windows.Forms.TableLayoutPanelGrowStyle.AddColumns;
            this.statusLightTable_tableLayoutPanel.Location = new System.Drawing.Point(0, 0);
            this.statusLightTable_tableLayoutPanel.Margin = new System.Windows.Forms.Padding(0);
            this.statusLightTable_tableLayoutPanel.Name = "statusLightTable_tableLayoutPanel";
            this.statusLightTable_tableLayoutPanel.RowCount = 1;
            this.statusLightTable_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.statusLightTable_tableLayoutPanel.Size = new System.Drawing.Size(461, 206);
            this.statusLightTable_tableLayoutPanel.TabIndex = 0;
            // 
            // errorTableDataBindingSource
            // 
            this.errorTableDataBindingSource.DataSource = typeof(Trafodion.Manager.Framework.Controls.ErrorTableData);
            // 
            // TrafodionSystemStatusUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.statusLightTable_tableLayoutPanel);
            this.Margin = new System.Windows.Forms.Padding(3, 0, 3, 3);
            this.Name = "TrafodionSystemStatusUserControl";
            this.Size = new System.Drawing.Size(461, 206);
            ((System.ComponentModel.ISupportInitialize)(this.socketClientBindingSource)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.socketClientBindingSource1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.errorTableDataBindingSource)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel statusLightTable_tableLayoutPanel;


        private System.Windows.Forms.BindingSource socketClientBindingSource;
        private System.Windows.Forms.BindingSource socketClientBindingSource1;
        private System.Windows.Forms.BindingSource errorTableDataBindingSource;

    }
}
