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
﻿namespace Trafodion.Manager.MetricMiner.Controls
{
    partial class WidgetPropertyInputControl
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
            MyDispose(disposing); 

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
            this._theTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._theGeneralTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theRequiredLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionButton1 = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theReportFileLocationComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.TrafodionLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theRequiredLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theServerVersion = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theWidgetTitle = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theVersion = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theWidgetName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theAuthor = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theDescription = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._theQueryInputTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theDrillDownTabPage = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.TrafodionLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theQueryInputControl = new Trafodion.Manager.MetricMiner.Controls.QueryInputControl();
            this._theDrillDownMappingUserControl = new Trafodion.Manager.MetricMiner.Controls.DrillDownMappingUserControl();
            this._theTabControl.SuspendLayout();
            this._theGeneralTabPage.SuspendLayout();
            this.TrafodionGroupBox2.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this._theQueryInputTabPage.SuspendLayout();
            this._theDrillDownTabPage.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theTabControl
            // 
            this._theTabControl.Controls.Add(this._theGeneralTabPage);
            this._theTabControl.Controls.Add(this._theQueryInputTabPage);
            this._theTabControl.Controls.Add(this._theDrillDownTabPage);
            this._theTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTabControl.HotTrack = true;
            this._theTabControl.Location = new System.Drawing.Point(0, 0);
            this._theTabControl.Multiline = true;
            this._theTabControl.Name = "_theTabControl";
            this._theTabControl.Padding = new System.Drawing.Point(10, 5);
            this._theTabControl.SelectedIndex = 0;
            this._theTabControl.Size = new System.Drawing.Size(590, 421);
            this._theTabControl.TabIndex = 0;
            this._theTabControl.SelectedIndexChanged += new System.EventHandler(this._theTabControl_SelectedIndexChanged);
            // 
            // _theGeneralTabPage
            // 
            this._theGeneralTabPage.Controls.Add(this._theRequiredLabel);
            this._theGeneralTabPage.Controls.Add(this.TrafodionGroupBox2);
            this._theGeneralTabPage.Controls.Add(this.TrafodionGroupBox1);
            this._theGeneralTabPage.Location = new System.Drawing.Point(4, 26);
            this._theGeneralTabPage.Name = "_theGeneralTabPage";
            this._theGeneralTabPage.Padding = new System.Windows.Forms.Padding(3);
            this._theGeneralTabPage.Size = new System.Drawing.Size(582, 391);
            this._theGeneralTabPage.TabIndex = 0;
            this._theGeneralTabPage.Text = "General Attributes";
            this._theGeneralTabPage.UseVisualStyleBackColor = true;
            // 
            // _theRequiredLabel
            // 
            this._theRequiredLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theRequiredLabel.AutoSize = true;
            this._theRequiredLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRequiredLabel.ForeColor = System.Drawing.Color.Red;
            this._theRequiredLabel.Location = new System.Drawing.Point(22, 371);
            this._theRequiredLabel.Name = "_theRequiredLabel";
            this._theRequiredLabel.Size = new System.Drawing.Size(62, 13);
            this._theRequiredLabel.TabIndex = 14;
            this._theRequiredLabel.Text = "* Required ";
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionGroupBox2.Controls.Add(this.TrafodionLabel8);
            this.TrafodionGroupBox2.Controls.Add(this.TrafodionButton1);
            this.TrafodionGroupBox2.Controls.Add(this._theReportFileLocationComboBox);
            this.TrafodionGroupBox2.Controls.Add(this.TrafodionLabel6);
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(19, 303);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(545, 65);
            this.TrafodionGroupBox2.TabIndex = 13;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "Report File Location";
            // 
            // TrafodionButton1
            // 
            this.TrafodionButton1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionButton1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionButton1.Location = new System.Drawing.Point(502, 26);
            this.TrafodionButton1.Name = "TrafodionButton1";
            this.TrafodionButton1.Size = new System.Drawing.Size(32, 23);
            this.TrafodionButton1.TabIndex = 5;
            this.TrafodionButton1.Text = "...";
            this.TrafodionButton1.UseVisualStyleBackColor = true;
            this.TrafodionButton1.Click += new System.EventHandler(this.TrafodionButton1_Click);
            // 
            // _theReportFileLocationComboBox
            // 
            this._theReportFileLocationComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theReportFileLocationComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theReportFileLocationComboBox.FormattingEnabled = true;
            this._theReportFileLocationComboBox.Location = new System.Drawing.Point(123, 26);
            this._theReportFileLocationComboBox.Name = "_theReportFileLocationComboBox";
            this._theReportFileLocationComboBox.Size = new System.Drawing.Size(373, 21);
            this._theReportFileLocationComboBox.TabIndex = 4;
            this._theReportFileLocationComboBox.TextChanged += new System.EventHandler(this._theReportFileLocationComboBox_TextChanged);
            // 
            // TrafodionLabel6
            // 
            this.TrafodionLabel6.AutoSize = true;
            this.TrafodionLabel6.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel6.Location = new System.Drawing.Point(76, 28);
            this.TrafodionLabel6.Name = "TrafodionLabel6";
            this.TrafodionLabel6.Size = new System.Drawing.Size(23, 13);
            this.TrafodionLabel6.TabIndex = 0;
            this.TrafodionLabel6.Text = "File";
            this.TrafodionLabel6.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionGroupBox1.Controls.Add(this._theRequiredLabel2);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel3);
            this.TrafodionGroupBox1.Controls.Add(this._theServerVersion);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel7);
            this.TrafodionGroupBox1.Controls.Add(this._theWidgetTitle);
            this.TrafodionGroupBox1.Controls.Add(this._theVersion);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel1);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel5);
            this.TrafodionGroupBox1.Controls.Add(this._theWidgetName);
            this.TrafodionGroupBox1.Controls.Add(this._theAuthor);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel2);
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionLabel4);
            this.TrafodionGroupBox1.Controls.Add(this._theDescription);
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(19, 3);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(545, 300);
            this.TrafodionGroupBox1.TabIndex = 12;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Report Configuration";
            // 
            // _theRequiredLabel2
            // 
            this._theRequiredLabel2.AutoSize = true;
            this._theRequiredLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRequiredLabel2.ForeColor = System.Drawing.Color.Red;
            this._theRequiredLabel2.Location = new System.Drawing.Point(101, 41);
            this._theRequiredLabel2.Name = "_theRequiredLabel2";
            this._theRequiredLabel2.Size = new System.Drawing.Size(13, 13);
            this._theRequiredLabel2.TabIndex = 13;
            this._theRequiredLabel2.Text = "*";
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(39, 88);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(60, 13);
            this.TrafodionLabel3.TabIndex = 12;
            this.TrafodionLabel3.Text = "Description";
            this.TrafodionLabel3.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theServerVersion
            // 
            this._theServerVersion.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theServerVersion.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theServerVersion.Location = new System.Drawing.Point(123, 257);
            this._theServerVersion.Name = "_theServerVersion";
            this._theServerVersion.Size = new System.Drawing.Size(411, 20);
            this._theServerVersion.TabIndex = 11;
            this._theServerVersion.TextChanged += new System.EventHandler(this._theServerVersion_TextChanged);
            // 
            // TrafodionLabel7
            // 
            this.TrafodionLabel7.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.TrafodionLabel7.AutoSize = true;
            this.TrafodionLabel7.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel7.Location = new System.Drawing.Point(22, 258);
            this.TrafodionLabel7.Name = "TrafodionLabel7";
            this.TrafodionLabel7.Size = new System.Drawing.Size(77, 13);
            this.TrafodionLabel7.TabIndex = 10;
            this.TrafodionLabel7.Text = "Server Version";
            this.TrafodionLabel7.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theWidgetTitle
            // 
            this._theWidgetTitle.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theWidgetTitle.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theWidgetTitle.Location = new System.Drawing.Point(123, 61);
            this._theWidgetTitle.Name = "_theWidgetTitle";
            this._theWidgetTitle.Size = new System.Drawing.Size(411, 20);
            this._theWidgetTitle.TabIndex = 3;
            this._theWidgetTitle.TextChanged += new System.EventHandler(this._theWidgetTitle_TextChanged);
            // 
            // _theVersion
            // 
            this._theVersion.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._theVersion.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theVersion.Location = new System.Drawing.Point(123, 231);
            this._theVersion.Name = "_theVersion";
            this._theVersion.Size = new System.Drawing.Size(186, 20);
            this._theVersion.TabIndex = 9;
            this._theVersion.TextChanged += new System.EventHandler(this._theVersion_TextChanged);
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(65, 38);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(34, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Name";
            this.TrafodionLabel1.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel5
            // 
            this.TrafodionLabel5.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.TrafodionLabel5.AutoSize = true;
            this.TrafodionLabel5.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel5.Location = new System.Drawing.Point(24, 232);
            this.TrafodionLabel5.Name = "TrafodionLabel5";
            this.TrafodionLabel5.Size = new System.Drawing.Size(75, 13);
            this.TrafodionLabel5.TabIndex = 8;
            this.TrafodionLabel5.Text = "Query Version";
            this.TrafodionLabel5.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theWidgetName
            // 
            this._theWidgetName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theWidgetName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theWidgetName.Location = new System.Drawing.Point(123, 35);
            this._theWidgetName.Name = "_theWidgetName";
            this._theWidgetName.Size = new System.Drawing.Size(411, 20);
            this._theWidgetName.TabIndex = 1;
            this._theWidgetName.TextChanged += new System.EventHandler(this._theWidgetName_TextChanged);
            // 
            // _theAuthor
            // 
            this._theAuthor.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theAuthor.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theAuthor.Location = new System.Drawing.Point(123, 205);
            this._theAuthor.Name = "_theAuthor";
            this._theAuthor.Size = new System.Drawing.Size(411, 20);
            this._theAuthor.TabIndex = 7;
            this._theAuthor.TextChanged += new System.EventHandler(this._theAuthor_TextChanged);
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(72, 64);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(27, 13);
            this.TrafodionLabel2.TabIndex = 2;
            this.TrafodionLabel2.Text = "Title";
            this.TrafodionLabel2.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel4.Location = new System.Drawing.Point(59, 207);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(40, 13);
            this.TrafodionLabel4.TabIndex = 6;
            this.TrafodionLabel4.Text = "Author";
            this.TrafodionLabel4.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theDescription
            // 
            this._theDescription.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theDescription.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theDescription.Location = new System.Drawing.Point(123, 87);
            this._theDescription.Name = "_theDescription";
            this._theDescription.Size = new System.Drawing.Size(411, 112);
            this._theDescription.TabIndex = 5;
            this._theDescription.Text = "";
            this._theDescription.TextChanged += new System.EventHandler(this._theDescription_TextChanged);
            // 
            // _theQueryInputTabPage
            // 
            this._theQueryInputTabPage.Controls.Add(this._theQueryInputControl);
            this._theQueryInputTabPage.Location = new System.Drawing.Point(4, 26);
            this._theQueryInputTabPage.Name = "_theQueryInputTabPage";
            this._theQueryInputTabPage.Size = new System.Drawing.Size(582, 391);
            this._theQueryInputTabPage.TabIndex = 2;
            this._theQueryInputTabPage.Text = "Query";
            this._theQueryInputTabPage.UseVisualStyleBackColor = true;
            // 
            // _theDrillDownTabPage
            // 
            this._theDrillDownTabPage.Controls.Add(this._theDrillDownMappingUserControl);
            this._theDrillDownTabPage.Location = new System.Drawing.Point(4, 26);
            this._theDrillDownTabPage.Name = "_theDrillDownTabPage";
            this._theDrillDownTabPage.Padding = new System.Windows.Forms.Padding(3);
            this._theDrillDownTabPage.Size = new System.Drawing.Size(582, 391);
            this._theDrillDownTabPage.TabIndex = 1;
            this._theDrillDownTabPage.Text = "Drill Down Attributes";
            this._theDrillDownTabPage.UseVisualStyleBackColor = true;
            // 
            // TrafodionLabel8
            // 
            this.TrafodionLabel8.AutoSize = true;
            this.TrafodionLabel8.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel8.ForeColor = System.Drawing.Color.Red;
            this.TrafodionLabel8.Location = new System.Drawing.Point(101, 32);
            this.TrafodionLabel8.Name = "TrafodionLabel8";
            this.TrafodionLabel8.Size = new System.Drawing.Size(13, 13);
            this.TrafodionLabel8.TabIndex = 14;
            this.TrafodionLabel8.Text = "*";
            // 
            // _theQueryInputControl
            // 
            this._theQueryInputControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryInputControl.Location = new System.Drawing.Point(0, 0);
            this._theQueryInputControl.Name = "_theQueryInputControl";
            this._theQueryInputControl.QueryText = "";
            this._theQueryInputControl.ReadOnly = false;
            this._theQueryInputControl.Size = new System.Drawing.Size(582, 391);
            this._theQueryInputControl.TabIndex = 0;
            // 
            // _theDrillDownMappingUserControl
            // 
            this._theDrillDownMappingUserControl.Config = null;
            this._theDrillDownMappingUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theDrillDownMappingUserControl.Location = new System.Drawing.Point(3, 3);
            this._theDrillDownMappingUserControl.Name = "_theDrillDownMappingUserControl";
            this._theDrillDownMappingUserControl.Size = new System.Drawing.Size(576, 385);
            this._theDrillDownMappingUserControl.TabIndex = 0;
            // 
            // WidgetPropertyInputControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theTabControl);
            this.Name = "WidgetPropertyInputControl";
            this.Size = new System.Drawing.Size(590, 421);
            this._theTabControl.ResumeLayout(false);
            this._theGeneralTabPage.ResumeLayout(false);
            this._theGeneralTabPage.PerformLayout();
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox2.PerformLayout();
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this._theQueryInputTabPage.ResumeLayout(false);
            this._theDrillDownTabPage.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _theTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theGeneralTabPage;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel6;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theServerVersion;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel7;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theWidgetTitle;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theVersion;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel5;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theWidgetName;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theAuthor;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel4;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _theDescription;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theDrillDownTabPage;
        private DrillDownMappingUserControl _theDrillDownMappingUserControl;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theQueryInputTabPage;
        private QueryInputControl _theQueryInputControl;
        private Trafodion.Manager.Framework.Controls.TrafodionButton TrafodionButton1;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theReportFileLocationComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theRequiredLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theRequiredLabel;
        private Framework.Controls.TrafodionLabel TrafodionLabel8;

    }
}
