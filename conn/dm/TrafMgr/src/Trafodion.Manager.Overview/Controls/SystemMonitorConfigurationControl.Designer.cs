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
namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class SystemMonitorConfigurationControl
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
            MyDispose(disposing);
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
            this._systemMonitorToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.panel35 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel13 = new System.Windows.Forms.FlowLayoutPanel();
            this.applyConfig_oneGuiButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.button25 = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel5 = new System.Windows.Forms.FlowLayoutPanel();
            this.checkBox24 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.checkBox25 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.checkBox26 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.panel9 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colorCpuQueue_ColButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.maxCPUQueue_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.checkCpuQueue_Checkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel8 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colorFreeMem_ColButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.maxFreeMem_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.checkBox6 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel7 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colorSwap_ColButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.maxSwap_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.checkBox5 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.scrollPanel_oneGuiPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.nsmTable_tableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.tableLayoutPanel4 = new System.Windows.Forms.TableLayoutPanel();
            this.panel4 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel4 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel5 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colCPUBSY_ColButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.metricCPUBSY_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel13 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.maxValCPUBSY_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.flowLayoutPanel7 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel6 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colDiskIO_ColButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.metricDiskIO_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel14 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.maxValDiskIO_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.flowLayoutPanel8 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel11 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colCacheHits_ColButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.metricCache_checkBox1 = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel15 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.maxCache_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.flowLayoutPanel9 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel12 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colDispatch_Button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.metricDispatch_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel16 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.maxDispatch_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.flowLayoutPanel10 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colSwap_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.metricSwap_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel17 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.maxValSwap_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.flowLayoutPanel11 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colFreeMem_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.metricFreeMem_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel18 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.maxValFreeMem_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.flowLayoutPanel12 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.colCPUQL_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.metricCPUQL_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel19 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.maxCPUQL_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.flowLayoutPanel14 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel10 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.label3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.flowLayoutPanel3 = new System.Windows.Forms.FlowLayoutPanel();
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.flowLayoutPanel15 = new System.Windows.Forms.FlowLayoutPanel();
            this.sysStatFlowLayout_flowLayoutPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.sysStatConn_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.sysStatDisk_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.sysStatTrans_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.panel20 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.sysStatAlarm_checkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.sysStatRedText_oneGuiLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.sysStatTopBottom_flowLayoutPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.sysMonBottom_radioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.sysStatTop_radioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.groupBox3 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.miscAggSegs_radioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.miscAggNone_radioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.groupBox12 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.miscToggleBG_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.groupBox6 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.miscShowSep_checkBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.gbTimelineOptions_groupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.miscTimelineMax_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label19 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.groupBox5 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.label21 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label20 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label22 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.colBGBar_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.colBGTimeline_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.colBGDiscon_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.groupBox10 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.colMiscMouse_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.label32 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.labelCPUDown = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label33 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.colMiscCPUDown_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.colMiscThreshExceeded_button = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.flowLayoutPanel6 = new System.Windows.Forms.FlowLayoutPanel();
            this.groupBox4 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.portNumINC_oneGuiMaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.portNum_oneGuiMaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this.portNumINC_Checkbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.label2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.groupBox11 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.refreshRate_MaskedTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMaskedTextBox();
            this._lblSeconds = new System.Windows.Forms.Label();
            this.label35 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.maxValCPUBSY_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.maxValDiskIO_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.maxCache_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.maxDispatch_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.maxValSwap_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.maxValFreeMem_TextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.maxCPUQL_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.refreshRate_textBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.label34 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.radioButton2 = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.radioButton5 = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.errorProvider1 = new System.Windows.Forms.ErrorProvider(this.components);
            this.panel35.SuspendLayout();
            this.flowLayoutPanel13.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.flowLayoutPanel5.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.panel9.SuspendLayout();
            this.panel8.SuspendLayout();
            this.panel7.SuspendLayout();
            this.scrollPanel_oneGuiPanel.SuspendLayout();
            this.nsmTable_tableLayoutPanel.SuspendLayout();
            this.tableLayoutPanel4.SuspendLayout();
            this.flowLayoutPanel4.SuspendLayout();
            this.panel5.SuspendLayout();
            this.panel13.SuspendLayout();
            this.flowLayoutPanel7.SuspendLayout();
            this.panel6.SuspendLayout();
            this.panel14.SuspendLayout();
            this.flowLayoutPanel8.SuspendLayout();
            this.panel11.SuspendLayout();
            this.panel15.SuspendLayout();
            this.flowLayoutPanel9.SuspendLayout();
            this.panel12.SuspendLayout();
            this.panel16.SuspendLayout();
            this.flowLayoutPanel10.SuspendLayout();
            this.panel3.SuspendLayout();
            this.panel17.SuspendLayout();
            this.flowLayoutPanel11.SuspendLayout();
            this.panel2.SuspendLayout();
            this.panel18.SuspendLayout();
            this.flowLayoutPanel12.SuspendLayout();
            this.panel1.SuspendLayout();
            this.panel19.SuspendLayout();
            this.flowLayoutPanel14.SuspendLayout();
            this.panel10.SuspendLayout();
            this.flowLayoutPanel3.SuspendLayout();
            this.oneGuiGroupBox1.SuspendLayout();
            this.flowLayoutPanel15.SuspendLayout();
            this.sysStatFlowLayout_flowLayoutPanel.SuspendLayout();
            this.panel20.SuspendLayout();
            this.oneGuiGroupBox2.SuspendLayout();
            this.sysStatTopBottom_flowLayoutPanel.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox12.SuspendLayout();
            this.groupBox6.SuspendLayout();
            this.gbTimelineOptions_groupBox.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox10.SuspendLayout();
            this.flowLayoutPanel6.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.groupBox11.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider1)).BeginInit();
            this.SuspendLayout();
            // 
            // _systemMonitorToolTip
            // 
            this._systemMonitorToolTip.IsBalloon = true;
            // 
            // panel35
            // 
            this.panel35.AutoSize = true;
            this.panel35.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel35.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panel35.Controls.Add(this.flowLayoutPanel13);
            this.panel35.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel35.Location = new System.Drawing.Point(0, 785);
            this.panel35.Name = "panel35";
            this.panel35.Size = new System.Drawing.Size(587, 34);
            this.panel35.TabIndex = 110;
            // 
            // flowLayoutPanel13
            // 
            this.flowLayoutPanel13.AutoSize = true;
            this.flowLayoutPanel13.Controls.Add(this.applyConfig_oneGuiButton);
            this.flowLayoutPanel13.Controls.Add(this.button25);
            this.flowLayoutPanel13.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel13.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel13.Name = "flowLayoutPanel13";
            this.flowLayoutPanel13.Size = new System.Drawing.Size(585, 32);
            this.flowLayoutPanel13.TabIndex = 138;
            // 
            // applyConfig_oneGuiButton
            // 
            this.applyConfig_oneGuiButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.applyConfig_oneGuiButton.Location = new System.Drawing.Point(3, 3);
            this.applyConfig_oneGuiButton.Name = "applyConfig_oneGuiButton";
            this.applyConfig_oneGuiButton.Size = new System.Drawing.Size(80, 26);
            this.applyConfig_oneGuiButton.TabIndex = 137;
            this.applyConfig_oneGuiButton.Text = "&Apply";
            this.applyConfig_oneGuiButton.UseVisualStyleBackColor = true;
            this.applyConfig_oneGuiButton.Click += new System.EventHandler(this.oneGuiButton1_Click);
            // 
            // button25
            // 
            this.button25.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button25.Location = new System.Drawing.Point(89, 3);
            this.button25.Name = "button25";
            this.button25.Size = new System.Drawing.Size(80, 26);
            this.button25.TabIndex = 112;
            this.button25.Text = "&Cancel";
            this.button25.UseVisualStyleBackColor = true;
            this.button25.Click += new System.EventHandler(this.button25_Click);
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.AutoSize = true;
            this.tableLayoutPanel2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel2.ColumnCount = 1;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.Controls.Add(this.flowLayoutPanel5, 0, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 2;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(200, 100);
            this.tableLayoutPanel2.TabIndex = 0;
            // 
            // flowLayoutPanel5
            // 
            this.flowLayoutPanel5.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.flowLayoutPanel5.AutoSize = true;
            this.flowLayoutPanel5.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel5.Controls.Add(this.checkBox24);
            this.flowLayoutPanel5.Controls.Add(this.checkBox25);
            this.flowLayoutPanel5.Controls.Add(this.checkBox26);
            this.flowLayoutPanel5.Location = new System.Drawing.Point(19, 3);
            this.flowLayoutPanel5.Name = "flowLayoutPanel5";
            this.flowLayoutPanel5.Size = new System.Drawing.Size(161, 14);
            this.flowLayoutPanel5.TabIndex = 0;
            // 
            // checkBox24
            // 
            this.checkBox24.AutoSize = true;
            this.checkBox24.Checked = true;
            this.checkBox24.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox24.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.checkBox24.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.checkBox24.Location = new System.Drawing.Point(3, 3);
            this.checkBox24.Name = "checkBox24";
            this.checkBox24.Size = new System.Drawing.Size(92, 18);
            this.checkBox24.TabIndex = 0;
            this.checkBox24.Text = "Connectivity";
            this.checkBox24.UseVisualStyleBackColor = true;
            // 
            // checkBox25
            // 
            this.checkBox25.AutoSize = true;
            this.checkBox25.Checked = true;
            this.checkBox25.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox25.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.checkBox25.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.checkBox25.Location = new System.Drawing.Point(3, 27);
            this.checkBox25.Name = "checkBox25";
            this.checkBox25.Size = new System.Drawing.Size(93, 18);
            this.checkBox25.TabIndex = 1;
            this.checkBox25.Text = "Transactions";
            this.checkBox25.UseVisualStyleBackColor = true;
            // 
            // checkBox26
            // 
            this.checkBox26.AutoSize = true;
            this.checkBox26.Checked = true;
            this.checkBox26.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox26.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.checkBox26.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.checkBox26.Location = new System.Drawing.Point(102, 27);
            this.checkBox26.Name = "checkBox26";
            this.checkBox26.Size = new System.Drawing.Size(56, 18);
            this.checkBox26.TabIndex = 2;
            this.checkBox26.Text = "Disks";
            this.checkBox26.UseVisualStyleBackColor = true;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.panel9, 0, 8);
            this.tableLayoutPanel1.Controls.Add(this.panel8, 0, 7);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 9;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(200, 100);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // panel9
            // 
            this.panel9.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.panel9.Controls.Add(this.colorCpuQueue_ColButton);
            this.panel9.Controls.Add(this.maxCPUQueue_textBox);
            this.panel9.Controls.Add(this.checkCpuQueue_Checkbox);
            this.panel9.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel9.Location = new System.Drawing.Point(0, 160);
            this.panel9.Margin = new System.Windows.Forms.Padding(0);
            this.panel9.Name = "panel9";
            this.panel9.Size = new System.Drawing.Size(200, 20);
            this.panel9.TabIndex = 116;
            // 
            // colorCpuQueue_ColButton
            // 
            this.colorCpuQueue_ColButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colorCpuQueue_ColButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colorCpuQueue_ColButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colorCpuQueue_ColButton.Location = new System.Drawing.Point(5, 3);
            this.colorCpuQueue_ColButton.Name = "colorCpuQueue_ColButton";
            this.colorCpuQueue_ColButton.Size = new System.Drawing.Size(27, 21);
            this.colorCpuQueue_ColButton.TabIndex = 110;
            this.colorCpuQueue_ColButton.UseVisualStyleBackColor = true;
            // 
            // maxCPUQueue_textBox
            // 
            this.maxCPUQueue_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxCPUQueue_textBox.Location = new System.Drawing.Point(182, 3);
            this.maxCPUQueue_textBox.Name = "maxCPUQueue_textBox";
            this.maxCPUQueue_textBox.Size = new System.Drawing.Size(103, 21);
            this.maxCPUQueue_textBox.TabIndex = 109;
            this.maxCPUQueue_textBox.TextChanged += new System.EventHandler(this.textBox_PasteValidate);
            this.maxCPUQueue_textBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBox_KeyPressValidate);
            // 
            // checkCpuQueue_Checkbox
            // 
            this.checkCpuQueue_Checkbox.AutoSize = true;
            this.checkCpuQueue_Checkbox.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.checkCpuQueue_Checkbox.Checked = true;
            this.checkCpuQueue_Checkbox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkCpuQueue_Checkbox.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.checkCpuQueue_Checkbox.FlatAppearance.BorderSize = 2;
            this.checkCpuQueue_Checkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.checkCpuQueue_Checkbox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkCpuQueue_Checkbox.Location = new System.Drawing.Point(38, 4);
            this.checkCpuQueue_Checkbox.Name = "checkCpuQueue_Checkbox";
            this.checkCpuQueue_Checkbox.Size = new System.Drawing.Size(150, 18);
            this.checkCpuQueue_Checkbox.TabIndex = 106;
            this.checkCpuQueue_Checkbox.Text = "CPU Queue Length";
            this.checkCpuQueue_Checkbox.UseVisualStyleBackColor = false;
            // 
            // panel8
            // 
            this.panel8.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel8.Controls.Add(this.colorFreeMem_ColButton);
            this.panel8.Controls.Add(this.maxFreeMem_textBox);
            this.panel8.Controls.Add(this.checkBox6);
            this.panel8.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel8.Location = new System.Drawing.Point(0, 140);
            this.panel8.Margin = new System.Windows.Forms.Padding(0);
            this.panel8.Name = "panel8";
            this.panel8.Size = new System.Drawing.Size(200, 20);
            this.panel8.TabIndex = 115;
            // 
            // colorFreeMem_ColButton
            // 
            this.colorFreeMem_ColButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colorFreeMem_ColButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colorFreeMem_ColButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colorFreeMem_ColButton.Location = new System.Drawing.Point(5, 2);
            this.colorFreeMem_ColButton.Name = "colorFreeMem_ColButton";
            this.colorFreeMem_ColButton.Size = new System.Drawing.Size(27, 21);
            this.colorFreeMem_ColButton.TabIndex = 110;
            this.colorFreeMem_ColButton.Text = "Click to Change";
            this.colorFreeMem_ColButton.UseVisualStyleBackColor = true;
            // 
            // maxFreeMem_textBox
            // 
            this.maxFreeMem_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxFreeMem_textBox.Location = new System.Drawing.Point(182, 3);
            this.maxFreeMem_textBox.Name = "maxFreeMem_textBox";
            this.maxFreeMem_textBox.Size = new System.Drawing.Size(103, 21);
            this.maxFreeMem_textBox.TabIndex = 109;
            this.maxFreeMem_textBox.TextChanged += new System.EventHandler(this.textBox_PasteValidate);
            this.maxFreeMem_textBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBox_KeyPressValidate);
            // 
            // checkBox6
            // 
            this.checkBox6.AutoSize = true;
            this.checkBox6.BackColor = System.Drawing.SystemColors.ControlLight;
            this.checkBox6.Checked = true;
            this.checkBox6.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox6.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.checkBox6.FlatAppearance.BorderSize = 2;
            this.checkBox6.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.checkBox6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox6.Location = new System.Drawing.Point(38, 4);
            this.checkBox6.Name = "checkBox6";
            this.checkBox6.Size = new System.Drawing.Size(118, 18);
            this.checkBox6.TabIndex = 106;
            this.checkBox6.Text = "Free Memory";
            this.checkBox6.UseVisualStyleBackColor = false;
            // 
            // panel7
            // 
            this.panel7.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.panel7.Controls.Add(this.colorSwap_ColButton);
            this.panel7.Controls.Add(this.maxSwap_textBox);
            this.panel7.Controls.Add(this.checkBox5);
            this.panel7.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel7.Location = new System.Drawing.Point(0, 150);
            this.panel7.Margin = new System.Windows.Forms.Padding(0);
            this.panel7.Name = "panel7";
            this.panel7.Size = new System.Drawing.Size(689, 20);
            this.panel7.TabIndex = 114;
            // 
            // colorSwap_ColButton
            // 
            this.colorSwap_ColButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colorSwap_ColButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colorSwap_ColButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colorSwap_ColButton.Location = new System.Drawing.Point(5, 2);
            this.colorSwap_ColButton.Name = "colorSwap_ColButton";
            this.colorSwap_ColButton.Size = new System.Drawing.Size(27, 21);
            this.colorSwap_ColButton.TabIndex = 110;
            this.colorSwap_ColButton.Text = "Click to Change";
            this.colorSwap_ColButton.UseVisualStyleBackColor = true;
            // 
            // maxSwap_textBox
            // 
            this.maxSwap_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxSwap_textBox.Location = new System.Drawing.Point(182, 3);
            this.maxSwap_textBox.Name = "maxSwap_textBox";
            this.maxSwap_textBox.Size = new System.Drawing.Size(103, 21);
            this.maxSwap_textBox.TabIndex = 109;
            this.maxSwap_textBox.TextChanged += new System.EventHandler(this.textBox_PasteValidate);
            this.maxSwap_textBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBox_KeyPressValidate);
            // 
            // checkBox5
            // 
            this.checkBox5.AutoSize = true;
            this.checkBox5.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.checkBox5.Checked = true;
            this.checkBox5.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox5.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.checkBox5.FlatAppearance.BorderSize = 2;
            this.checkBox5.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.checkBox5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox5.Location = new System.Drawing.Point(38, 4);
            this.checkBox5.Name = "checkBox5";
            this.checkBox5.Size = new System.Drawing.Size(66, 18);
            this.checkBox5.TabIndex = 106;
            this.checkBox5.Text = "Swap";
            this.checkBox5.UseVisualStyleBackColor = false;
            // 
            // scrollPanel_oneGuiPanel
            // 
            this.scrollPanel_oneGuiPanel.AutoScroll = true;
            this.scrollPanel_oneGuiPanel.BackColor = System.Drawing.SystemColors.ControlLightLight;
            this.scrollPanel_oneGuiPanel.Controls.Add(this.nsmTable_tableLayoutPanel);
            this.scrollPanel_oneGuiPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.scrollPanel_oneGuiPanel.Location = new System.Drawing.Point(0, 0);
            this.scrollPanel_oneGuiPanel.Name = "scrollPanel_oneGuiPanel";
            this.scrollPanel_oneGuiPanel.Size = new System.Drawing.Size(587, 785);
            this.scrollPanel_oneGuiPanel.TabIndex = 144;
            this.scrollPanel_oneGuiPanel.Scroll += new System.Windows.Forms.ScrollEventHandler(this.scrollPanel_oneGuiPanel_Scroll);
            // 
            // nsmTable_tableLayoutPanel
            // 
            this.nsmTable_tableLayoutPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.nsmTable_tableLayoutPanel.AutoSize = true;
            this.nsmTable_tableLayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.nsmTable_tableLayoutPanel.BackColor = System.Drawing.SystemColors.ControlLightLight;
            this.nsmTable_tableLayoutPanel.CellBorderStyle = System.Windows.Forms.TableLayoutPanelCellBorderStyle.Single;
            this.nsmTable_tableLayoutPanel.ColumnCount = 1;
            this.nsmTable_tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.nsmTable_tableLayoutPanel.Controls.Add(this.tableLayoutPanel4, 0, 2);
            this.nsmTable_tableLayoutPanel.Controls.Add(this.flowLayoutPanel3, 0, 1);
            this.nsmTable_tableLayoutPanel.Controls.Add(this.flowLayoutPanel1, 0, 4);
            this.nsmTable_tableLayoutPanel.Controls.Add(this.flowLayoutPanel2, 0, 3);
            this.nsmTable_tableLayoutPanel.Controls.Add(this.flowLayoutPanel6, 0, 0);
            this.nsmTable_tableLayoutPanel.Location = new System.Drawing.Point(0, 0);
            this.nsmTable_tableLayoutPanel.Name = "nsmTable_tableLayoutPanel";
            this.nsmTable_tableLayoutPanel.RowCount = 5;
            this.nsmTable_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.nsmTable_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.nsmTable_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.nsmTable_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.nsmTable_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.nsmTable_tableLayoutPanel.Size = new System.Drawing.Size(568, 687);
            this.nsmTable_tableLayoutPanel.TabIndex = 143;
            // 
            // tableLayoutPanel4
            // 
            this.tableLayoutPanel4.AutoSize = true;
            this.tableLayoutPanel4.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel4.ColumnCount = 1;
            this.tableLayoutPanel4.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel4.Controls.Add(this.panel4, 0, 1);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel4, 0, 2);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel7, 0, 3);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel8, 0, 4);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel9, 0, 5);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel10, 0, 6);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel11, 0, 7);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel12, 0, 8);
            this.tableLayoutPanel4.Controls.Add(this.flowLayoutPanel14, 0, 0);
            this.tableLayoutPanel4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel4.Location = new System.Drawing.Point(1, 206);
            this.tableLayoutPanel4.Margin = new System.Windows.Forms.Padding(0);
            this.tableLayoutPanel4.Name = "tableLayoutPanel4";
            this.tableLayoutPanel4.RowCount = 9;
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 23F));
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel4.Size = new System.Drawing.Size(566, 279);
            this.tableLayoutPanel4.TabIndex = 145;
            // 
            // panel4
            // 
            this.panel4.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel4.Location = new System.Drawing.Point(3, 26);
            this.panel4.Name = "panel4";
            this.panel4.Size = new System.Drawing.Size(560, 26);
            this.panel4.TabIndex = 118;
            // 
            // flowLayoutPanel4
            // 
            this.flowLayoutPanel4.AutoSize = true;
            this.flowLayoutPanel4.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel4.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.flowLayoutPanel4.Controls.Add(this.panel5);
            this.flowLayoutPanel4.Controls.Add(this.panel13);
            this.flowLayoutPanel4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel4.Location = new System.Drawing.Point(3, 58);
            this.flowLayoutPanel4.Name = "flowLayoutPanel4";
            this.flowLayoutPanel4.Size = new System.Drawing.Size(560, 26);
            this.flowLayoutPanel4.TabIndex = 119;
            // 
            // panel5
            // 
            this.panel5.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.panel5.Controls.Add(this.colCPUBSY_ColButton);
            this.panel5.Controls.Add(this.metricCPUBSY_checkBox);
            this.panel5.Location = new System.Drawing.Point(0, 0);
            this.panel5.Margin = new System.Windows.Forms.Padding(0);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(180, 23);
            this.panel5.TabIndex = 111;
            // 
            // colCPUBSY_ColButton
            // 
            this.colCPUBSY_ColButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colCPUBSY_ColButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colCPUBSY_ColButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colCPUBSY_ColButton.Location = new System.Drawing.Point(11, 2);
            this.colCPUBSY_ColButton.Name = "colCPUBSY_ColButton";
            this.colCPUBSY_ColButton.Size = new System.Drawing.Size(27, 21);
            this.colCPUBSY_ColButton.TabIndex = 111;
            this.colCPUBSY_ColButton.UseVisualStyleBackColor = true;
            this.colCPUBSY_ColButton.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // metricCPUBSY_checkBox
            // 
            this.metricCPUBSY_checkBox.AutoSize = true;
            this.metricCPUBSY_checkBox.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.metricCPUBSY_checkBox.Checked = true;
            this.metricCPUBSY_checkBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.metricCPUBSY_checkBox.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.metricCPUBSY_checkBox.FlatAppearance.BorderSize = 2;
            this.metricCPUBSY_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.metricCPUBSY_checkBox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricCPUBSY_checkBox.Location = new System.Drawing.Point(44, 4);
            this.metricCPUBSY_checkBox.Name = "metricCPUBSY_checkBox";
            this.metricCPUBSY_checkBox.Size = new System.Drawing.Size(110, 18);
            this.metricCPUBSY_checkBox.TabIndex = 106;
            this.metricCPUBSY_checkBox.Text = "% CPU Busy";
            this.metricCPUBSY_checkBox.UseVisualStyleBackColor = false;
            // 
            // panel13
            // 
            this.panel13.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel13.Controls.Add(this.maxValCPUBSY_MaskedTextBox);
            this.panel13.Location = new System.Drawing.Point(183, 3);
            this.panel13.Name = "panel13";
            this.panel13.Size = new System.Drawing.Size(173, 20);
            this.panel13.TabIndex = 112;
            // 
            // maxValCPUBSY_MaskedTextBox
            // 
            this.maxValCPUBSY_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.maxValCPUBSY_MaskedTextBox.Location = new System.Drawing.Point(43, 0);
            this.maxValCPUBSY_MaskedTextBox.Mask = "000";
            this.maxValCPUBSY_MaskedTextBox.Name = "maxValCPUBSY_MaskedTextBox";
            this.maxValCPUBSY_MaskedTextBox.PromptChar = ' ';
            this.maxValCPUBSY_MaskedTextBox.Size = new System.Drawing.Size(103, 20);
            this.maxValCPUBSY_MaskedTextBox.TabIndex = 8;
            this.maxValCPUBSY_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.maxValCPUBSY_MaskedTextBox.TextChanged += new System.EventHandler(this.maxValCPUBSY_MaskedTextBox_TextChanged);
            // 
            // flowLayoutPanel7
            // 
            this.flowLayoutPanel7.AutoSize = true;
            this.flowLayoutPanel7.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel7.BackColor = System.Drawing.SystemColors.ControlLight;
            this.flowLayoutPanel7.Controls.Add(this.panel6);
            this.flowLayoutPanel7.Controls.Add(this.panel14);
            this.flowLayoutPanel7.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel7.Location = new System.Drawing.Point(3, 90);
            this.flowLayoutPanel7.Name = "flowLayoutPanel7";
            this.flowLayoutPanel7.Size = new System.Drawing.Size(560, 26);
            this.flowLayoutPanel7.TabIndex = 120;
            // 
            // panel6
            // 
            this.panel6.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel6.Controls.Add(this.colDiskIO_ColButton);
            this.panel6.Controls.Add(this.metricDiskIO_checkBox);
            this.panel6.Location = new System.Drawing.Point(0, 0);
            this.panel6.Margin = new System.Windows.Forms.Padding(0);
            this.panel6.Name = "panel6";
            this.panel6.Size = new System.Drawing.Size(180, 23);
            this.panel6.TabIndex = 112;
            // 
            // colDiskIO_ColButton
            // 
            this.colDiskIO_ColButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colDiskIO_ColButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colDiskIO_ColButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colDiskIO_ColButton.Location = new System.Drawing.Point(11, 2);
            this.colDiskIO_ColButton.Name = "colDiskIO_ColButton";
            this.colDiskIO_ColButton.Size = new System.Drawing.Size(27, 21);
            this.colDiskIO_ColButton.TabIndex = 110;
            this.colDiskIO_ColButton.UseVisualStyleBackColor = true;
            this.colDiskIO_ColButton.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // metricDiskIO_checkBox
            // 
            this.metricDiskIO_checkBox.AutoSize = true;
            this.metricDiskIO_checkBox.BackColor = System.Drawing.SystemColors.ControlLight;
            this.metricDiskIO_checkBox.Checked = true;
            this.metricDiskIO_checkBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.metricDiskIO_checkBox.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.metricDiskIO_checkBox.FlatAppearance.BorderSize = 2;
            this.metricDiskIO_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.metricDiskIO_checkBox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricDiskIO_checkBox.Location = new System.Drawing.Point(44, 4);
            this.metricDiskIO_checkBox.Name = "metricDiskIO_checkBox";
            this.metricDiskIO_checkBox.Size = new System.Drawing.Size(87, 18);
            this.metricDiskIO_checkBox.TabIndex = 106;
            this.metricDiskIO_checkBox.Text = "Disk I/O";
            this.metricDiskIO_checkBox.UseVisualStyleBackColor = false;
            // 
            // panel14
            // 
            this.panel14.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel14.Controls.Add(this.maxValDiskIO_MaskedTextBox);
            this.panel14.Location = new System.Drawing.Point(183, 3);
            this.panel14.Name = "panel14";
            this.panel14.Size = new System.Drawing.Size(173, 20);
            this.panel14.TabIndex = 113;
            // 
            // maxValDiskIO_MaskedTextBox
            // 
            this.maxValDiskIO_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.maxValDiskIO_MaskedTextBox.Location = new System.Drawing.Point(43, 0);
            this.maxValDiskIO_MaskedTextBox.Mask = "00000000";
            this.maxValDiskIO_MaskedTextBox.Name = "maxValDiskIO_MaskedTextBox";
            this.maxValDiskIO_MaskedTextBox.PromptChar = ' ';
            this.maxValDiskIO_MaskedTextBox.Size = new System.Drawing.Size(103, 20);
            this.maxValDiskIO_MaskedTextBox.TabIndex = 9;
            this.maxValDiskIO_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.maxValDiskIO_MaskedTextBox.TextChanged += new System.EventHandler(this.maxValDiskIO_MaskedTextBox_TextChanged);
            // 
            // flowLayoutPanel8
            // 
            this.flowLayoutPanel8.AutoSize = true;
            this.flowLayoutPanel8.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel8.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.flowLayoutPanel8.Controls.Add(this.panel11);
            this.flowLayoutPanel8.Controls.Add(this.panel15);
            this.flowLayoutPanel8.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel8.Location = new System.Drawing.Point(3, 122);
            this.flowLayoutPanel8.Name = "flowLayoutPanel8";
            this.flowLayoutPanel8.Size = new System.Drawing.Size(560, 26);
            this.flowLayoutPanel8.TabIndex = 121;
            // 
            // panel11
            // 
            this.panel11.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.panel11.Controls.Add(this.colCacheHits_ColButton);
            this.panel11.Controls.Add(this.metricCache_checkBox1);
            this.panel11.Location = new System.Drawing.Point(0, 0);
            this.panel11.Margin = new System.Windows.Forms.Padding(0);
            this.panel11.Name = "panel11";
            this.panel11.Size = new System.Drawing.Size(180, 23);
            this.panel11.TabIndex = 113;
            // 
            // colCacheHits_ColButton
            // 
            this.colCacheHits_ColButton.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colCacheHits_ColButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colCacheHits_ColButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colCacheHits_ColButton.Location = new System.Drawing.Point(11, 2);
            this.colCacheHits_ColButton.Name = "colCacheHits_ColButton";
            this.colCacheHits_ColButton.Size = new System.Drawing.Size(27, 21);
            this.colCacheHits_ColButton.TabIndex = 110;
            this.colCacheHits_ColButton.UseVisualStyleBackColor = true;
            this.colCacheHits_ColButton.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // metricCache_checkBox1
            // 
            this.metricCache_checkBox1.AutoSize = true;
            this.metricCache_checkBox1.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.metricCache_checkBox1.Checked = true;
            this.metricCache_checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
            this.metricCache_checkBox1.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.metricCache_checkBox1.FlatAppearance.BorderSize = 2;
            this.metricCache_checkBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.metricCache_checkBox1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricCache_checkBox1.Location = new System.Drawing.Point(44, 4);
            this.metricCache_checkBox1.Name = "metricCache_checkBox1";
            this.metricCache_checkBox1.Size = new System.Drawing.Size(100, 18);
            this.metricCache_checkBox1.TabIndex = 106;
            this.metricCache_checkBox1.Text = "Cache Hits";
            this.metricCache_checkBox1.UseVisualStyleBackColor = false;
            // 
            // panel15
            // 
            this.panel15.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel15.Controls.Add(this.maxCache_MaskedTextBox);
            this.panel15.Location = new System.Drawing.Point(183, 3);
            this.panel15.Name = "panel15";
            this.panel15.Size = new System.Drawing.Size(173, 20);
            this.panel15.TabIndex = 114;
            // 
            // maxCache_MaskedTextBox
            // 
            this.maxCache_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.maxCache_MaskedTextBox.Location = new System.Drawing.Point(43, 0);
            this.maxCache_MaskedTextBox.Mask = "00000000";
            this.maxCache_MaskedTextBox.Name = "maxCache_MaskedTextBox";
            this.maxCache_MaskedTextBox.PromptChar = ' ';
            this.maxCache_MaskedTextBox.Size = new System.Drawing.Size(103, 20);
            this.maxCache_MaskedTextBox.TabIndex = 10;
            this.maxCache_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.maxCache_MaskedTextBox.TextChanged += new System.EventHandler(this.maxCache_MaskedTextBox_TextChanged);
            // 
            // flowLayoutPanel9
            // 
            this.flowLayoutPanel9.AutoSize = true;
            this.flowLayoutPanel9.BackColor = System.Drawing.SystemColors.ControlLight;
            this.flowLayoutPanel9.Controls.Add(this.panel12);
            this.flowLayoutPanel9.Controls.Add(this.panel16);
            this.flowLayoutPanel9.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel9.Location = new System.Drawing.Point(3, 154);
            this.flowLayoutPanel9.Name = "flowLayoutPanel9";
            this.flowLayoutPanel9.Size = new System.Drawing.Size(560, 26);
            this.flowLayoutPanel9.TabIndex = 122;
            // 
            // panel12
            // 
            this.panel12.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel12.Controls.Add(this.colDispatch_Button);
            this.panel12.Controls.Add(this.metricDispatch_checkBox);
            this.panel12.Location = new System.Drawing.Point(0, 0);
            this.panel12.Margin = new System.Windows.Forms.Padding(0);
            this.panel12.Name = "panel12";
            this.panel12.Size = new System.Drawing.Size(180, 23);
            this.panel12.TabIndex = 114;
            // 
            // colDispatch_Button
            // 
            this.colDispatch_Button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colDispatch_Button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colDispatch_Button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colDispatch_Button.Location = new System.Drawing.Point(11, 2);
            this.colDispatch_Button.Name = "colDispatch_Button";
            this.colDispatch_Button.Size = new System.Drawing.Size(27, 21);
            this.colDispatch_Button.TabIndex = 110;
            this.colDispatch_Button.UseVisualStyleBackColor = true;
            this.colDispatch_Button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // metricDispatch_checkBox
            // 
            this.metricDispatch_checkBox.AutoSize = true;
            this.metricDispatch_checkBox.BackColor = System.Drawing.SystemColors.ControlLight;
            this.metricDispatch_checkBox.Checked = true;
            this.metricDispatch_checkBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.metricDispatch_checkBox.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.metricDispatch_checkBox.FlatAppearance.BorderSize = 2;
            this.metricDispatch_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.metricDispatch_checkBox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricDispatch_checkBox.Location = new System.Drawing.Point(44, 4);
            this.metricDispatch_checkBox.Name = "metricDispatch_checkBox";
            this.metricDispatch_checkBox.Size = new System.Drawing.Size(88, 18);
            this.metricDispatch_checkBox.TabIndex = 106;
            this.metricDispatch_checkBox.Text = "Dispatch";
            this.metricDispatch_checkBox.UseVisualStyleBackColor = false;
            // 
            // panel16
            // 
            this.panel16.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel16.Controls.Add(this.maxDispatch_MaskedTextBox);
            this.panel16.Location = new System.Drawing.Point(183, 3);
            this.panel16.Name = "panel16";
            this.panel16.Size = new System.Drawing.Size(173, 20);
            this.panel16.TabIndex = 115;
            // 
            // maxDispatch_MaskedTextBox
            // 
            this.maxDispatch_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.maxDispatch_MaskedTextBox.Location = new System.Drawing.Point(43, 0);
            this.maxDispatch_MaskedTextBox.Mask = "00000000";
            this.maxDispatch_MaskedTextBox.Name = "maxDispatch_MaskedTextBox";
            this.maxDispatch_MaskedTextBox.PromptChar = ' ';
            this.maxDispatch_MaskedTextBox.Size = new System.Drawing.Size(103, 20);
            this.maxDispatch_MaskedTextBox.TabIndex = 11;
            this.maxDispatch_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.maxDispatch_MaskedTextBox.TextChanged += new System.EventHandler(this.maxDispatch_MaskedTextBox_TextChanged);
            // 
            // flowLayoutPanel10
            // 
            this.flowLayoutPanel10.AutoSize = true;
            this.flowLayoutPanel10.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel10.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.flowLayoutPanel10.Controls.Add(this.panel3);
            this.flowLayoutPanel10.Controls.Add(this.panel17);
            this.flowLayoutPanel10.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel10.Location = new System.Drawing.Point(3, 186);
            this.flowLayoutPanel10.Name = "flowLayoutPanel10";
            this.flowLayoutPanel10.Size = new System.Drawing.Size(560, 26);
            this.flowLayoutPanel10.TabIndex = 123;
            // 
            // panel3
            // 
            this.panel3.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.panel3.Controls.Add(this.colSwap_button);
            this.panel3.Controls.Add(this.metricSwap_checkBox);
            this.panel3.Location = new System.Drawing.Point(0, 0);
            this.panel3.Margin = new System.Windows.Forms.Padding(0);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(180, 23);
            this.panel3.TabIndex = 115;
            // 
            // colSwap_button
            // 
            this.colSwap_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colSwap_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colSwap_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colSwap_button.Location = new System.Drawing.Point(11, 2);
            this.colSwap_button.Name = "colSwap_button";
            this.colSwap_button.Size = new System.Drawing.Size(27, 21);
            this.colSwap_button.TabIndex = 110;
            this.colSwap_button.UseVisualStyleBackColor = true;
            this.colSwap_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // metricSwap_checkBox
            // 
            this.metricSwap_checkBox.AutoSize = true;
            this.metricSwap_checkBox.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.metricSwap_checkBox.Checked = true;
            this.metricSwap_checkBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.metricSwap_checkBox.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.metricSwap_checkBox.FlatAppearance.BorderSize = 2;
            this.metricSwap_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.metricSwap_checkBox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricSwap_checkBox.Location = new System.Drawing.Point(44, 4);
            this.metricSwap_checkBox.Name = "metricSwap_checkBox";
            this.metricSwap_checkBox.Size = new System.Drawing.Size(66, 18);
            this.metricSwap_checkBox.TabIndex = 106;
            this.metricSwap_checkBox.Text = "Swap";
            this.metricSwap_checkBox.UseVisualStyleBackColor = false;
            // 
            // panel17
            // 
            this.panel17.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel17.Controls.Add(this.maxValSwap_MaskedTextBox);
            this.panel17.Location = new System.Drawing.Point(183, 3);
            this.panel17.Name = "panel17";
            this.panel17.Size = new System.Drawing.Size(173, 20);
            this.panel17.TabIndex = 116;
            // 
            // maxValSwap_MaskedTextBox
            // 
            this.maxValSwap_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.maxValSwap_MaskedTextBox.Location = new System.Drawing.Point(43, 0);
            this.maxValSwap_MaskedTextBox.Mask = "00000000";
            this.maxValSwap_MaskedTextBox.Name = "maxValSwap_MaskedTextBox";
            this.maxValSwap_MaskedTextBox.PromptChar = ' ';
            this.maxValSwap_MaskedTextBox.Size = new System.Drawing.Size(103, 20);
            this.maxValSwap_MaskedTextBox.TabIndex = 12;
            this.maxValSwap_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.maxValSwap_MaskedTextBox.TextChanged += new System.EventHandler(this.maxValSwap_MaskedTextBox_TextChanged);
            // 
            // flowLayoutPanel11
            // 
            this.flowLayoutPanel11.AutoSize = true;
            this.flowLayoutPanel11.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel11.BackColor = System.Drawing.SystemColors.ControlLight;
            this.flowLayoutPanel11.Controls.Add(this.panel2);
            this.flowLayoutPanel11.Controls.Add(this.panel18);
            this.flowLayoutPanel11.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel11.Location = new System.Drawing.Point(3, 218);
            this.flowLayoutPanel11.Name = "flowLayoutPanel11";
            this.flowLayoutPanel11.Size = new System.Drawing.Size(560, 26);
            this.flowLayoutPanel11.TabIndex = 124;
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.SystemColors.ControlLight;
            this.panel2.Controls.Add(this.colFreeMem_button);
            this.panel2.Controls.Add(this.metricFreeMem_checkBox);
            this.panel2.Location = new System.Drawing.Point(0, 0);
            this.panel2.Margin = new System.Windows.Forms.Padding(0);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(180, 23);
            this.panel2.TabIndex = 116;
            // 
            // colFreeMem_button
            // 
            this.colFreeMem_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colFreeMem_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colFreeMem_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colFreeMem_button.Location = new System.Drawing.Point(11, 2);
            this.colFreeMem_button.Name = "colFreeMem_button";
            this.colFreeMem_button.Size = new System.Drawing.Size(27, 21);
            this.colFreeMem_button.TabIndex = 110;
            this.colFreeMem_button.UseVisualStyleBackColor = true;
            this.colFreeMem_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // metricFreeMem_checkBox
            // 
            this.metricFreeMem_checkBox.AutoSize = true;
            this.metricFreeMem_checkBox.BackColor = System.Drawing.SystemColors.ControlLight;
            this.metricFreeMem_checkBox.Checked = true;
            this.metricFreeMem_checkBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.metricFreeMem_checkBox.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.metricFreeMem_checkBox.FlatAppearance.BorderSize = 2;
            this.metricFreeMem_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.metricFreeMem_checkBox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricFreeMem_checkBox.Location = new System.Drawing.Point(44, 4);
            this.metricFreeMem_checkBox.Name = "metricFreeMem_checkBox";
            this.metricFreeMem_checkBox.Size = new System.Drawing.Size(118, 18);
            this.metricFreeMem_checkBox.TabIndex = 106;
            this.metricFreeMem_checkBox.Text = "Free Memory";
            this.metricFreeMem_checkBox.UseVisualStyleBackColor = false;
            // 
            // panel18
            // 
            this.panel18.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel18.Controls.Add(this.maxValFreeMem_MaskedTextBox);
            this.panel18.Location = new System.Drawing.Point(183, 3);
            this.panel18.Name = "panel18";
            this.panel18.Size = new System.Drawing.Size(173, 20);
            this.panel18.TabIndex = 117;
            // 
            // maxValFreeMem_MaskedTextBox
            // 
            this.maxValFreeMem_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.maxValFreeMem_MaskedTextBox.Location = new System.Drawing.Point(43, 0);
            this.maxValFreeMem_MaskedTextBox.Mask = "00000000";
            this.maxValFreeMem_MaskedTextBox.Name = "maxValFreeMem_MaskedTextBox";
            this.maxValFreeMem_MaskedTextBox.PromptChar = ' ';
            this.maxValFreeMem_MaskedTextBox.Size = new System.Drawing.Size(103, 20);
            this.maxValFreeMem_MaskedTextBox.TabIndex = 13;
            this.maxValFreeMem_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.maxValFreeMem_MaskedTextBox.TextChanged += new System.EventHandler(this.maxValFreeMem_MaskedTextBox_TextChanged);
            // 
            // flowLayoutPanel12
            // 
            this.flowLayoutPanel12.AutoSize = true;
            this.flowLayoutPanel12.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel12.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.flowLayoutPanel12.Controls.Add(this.panel1);
            this.flowLayoutPanel12.Controls.Add(this.panel19);
            this.flowLayoutPanel12.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel12.Location = new System.Drawing.Point(3, 250);
            this.flowLayoutPanel12.Name = "flowLayoutPanel12";
            this.flowLayoutPanel12.Size = new System.Drawing.Size(560, 26);
            this.flowLayoutPanel12.TabIndex = 125;
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.panel1.Controls.Add(this.colCPUQL_button);
            this.panel1.Controls.Add(this.metricCPUQL_checkBox);
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Margin = new System.Windows.Forms.Padding(0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(180, 23);
            this.panel1.TabIndex = 117;
            // 
            // colCPUQL_button
            // 
            this.colCPUQL_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colCPUQL_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colCPUQL_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colCPUQL_button.Location = new System.Drawing.Point(11, 2);
            this.colCPUQL_button.Name = "colCPUQL_button";
            this.colCPUQL_button.Size = new System.Drawing.Size(27, 21);
            this.colCPUQL_button.TabIndex = 110;
            this.colCPUQL_button.UseVisualStyleBackColor = true;
            this.colCPUQL_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // metricCPUQL_checkBox
            // 
            this.metricCPUQL_checkBox.AutoSize = true;
            this.metricCPUQL_checkBox.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.metricCPUQL_checkBox.Checked = true;
            this.metricCPUQL_checkBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.metricCPUQL_checkBox.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.metricCPUQL_checkBox.FlatAppearance.BorderSize = 2;
            this.metricCPUQL_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.metricCPUQL_checkBox.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metricCPUQL_checkBox.Location = new System.Drawing.Point(44, 4);
            this.metricCPUQL_checkBox.Name = "metricCPUQL_checkBox";
            this.metricCPUQL_checkBox.Size = new System.Drawing.Size(150, 18);
            this.metricCPUQL_checkBox.TabIndex = 106;
            this.metricCPUQL_checkBox.Text = "CPU Queue Length";
            this.metricCPUQL_checkBox.UseVisualStyleBackColor = false;
            // 
            // panel19
            // 
            this.panel19.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel19.Controls.Add(this.maxCPUQL_MaskedTextBox);
            this.panel19.Location = new System.Drawing.Point(183, 3);
            this.panel19.Name = "panel19";
            this.panel19.Size = new System.Drawing.Size(173, 20);
            this.panel19.TabIndex = 118;
            // 
            // maxCPUQL_MaskedTextBox
            // 
            this.maxCPUQL_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.maxCPUQL_MaskedTextBox.Location = new System.Drawing.Point(43, 0);
            this.maxCPUQL_MaskedTextBox.Mask = "00000000";
            this.maxCPUQL_MaskedTextBox.Name = "maxCPUQL_MaskedTextBox";
            this.maxCPUQL_MaskedTextBox.PromptChar = ' ';
            this.maxCPUQL_MaskedTextBox.Size = new System.Drawing.Size(103, 20);
            this.maxCPUQL_MaskedTextBox.TabIndex = 14;
            this.maxCPUQL_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.maxCPUQL_MaskedTextBox.TextChanged += new System.EventHandler(this.maxCPUQL_MaskedTextBox_TextChanged);
            // 
            // flowLayoutPanel14
            // 
            this.flowLayoutPanel14.AutoSize = true;
            this.flowLayoutPanel14.Controls.Add(this.panel10);
            this.flowLayoutPanel14.Controls.Add(this.label4);
            this.flowLayoutPanel14.Location = new System.Drawing.Point(3, 3);
            this.flowLayoutPanel14.Name = "flowLayoutPanel14";
            this.flowLayoutPanel14.Size = new System.Drawing.Size(336, 17);
            this.flowLayoutPanel14.TabIndex = 2;
            // 
            // panel10
            // 
            this.panel10.BackColor = System.Drawing.SystemColors.ControlLightLight;
            this.panel10.Controls.Add(this.label3);
            this.panel10.Location = new System.Drawing.Point(0, 0);
            this.panel10.Margin = new System.Windows.Forms.Padding(0);
            this.panel10.Name = "panel10";
            this.panel10.Size = new System.Drawing.Size(222, 20);
            this.panel10.TabIndex = 117;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 9.75F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Underline))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(58, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(57, 16);
            this.label3.TabIndex = 0;
            this.label3.Text = "Metric ";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Verdana", 9.75F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Underline))), System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(225, 0);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(108, 16);
            this.label4.TabIndex = 1;
            this.label4.Text = "100% Setting";
            // 
            // flowLayoutPanel3
            // 
            this.flowLayoutPanel3.AutoSize = true;
            this.flowLayoutPanel3.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel3.Controls.Add(this.oneGuiGroupBox1);
            this.flowLayoutPanel3.Controls.Add(this.oneGuiGroupBox2);
            this.flowLayoutPanel3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel3.Location = new System.Drawing.Point(4, 83);
            this.flowLayoutPanel3.Name = "flowLayoutPanel3";
            this.flowLayoutPanel3.Size = new System.Drawing.Size(560, 119);
            this.flowLayoutPanel3.TabIndex = 142;
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.AutoSize = true;
            this.oneGuiGroupBox1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.oneGuiGroupBox1.Controls.Add(this.flowLayoutPanel15);
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(3, 3);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(341, 63);
            this.oneGuiGroupBox1.TabIndex = 3;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Active System Status Alerts";
            // 
            // flowLayoutPanel15
            // 
            this.flowLayoutPanel15.AutoSize = true;
            this.flowLayoutPanel15.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel15.Controls.Add(this.sysStatFlowLayout_flowLayoutPanel);
            this.flowLayoutPanel15.Controls.Add(this.sysStatRedText_oneGuiLabel);
            this.flowLayoutPanel15.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel15.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanel15.Location = new System.Drawing.Point(3, 17);
            this.flowLayoutPanel15.Name = "flowLayoutPanel15";
            this.flowLayoutPanel15.Size = new System.Drawing.Size(335, 43);
            this.flowLayoutPanel15.TabIndex = 6;
            // 
            // sysStatFlowLayout_flowLayoutPanel
            // 
            this.sysStatFlowLayout_flowLayoutPanel.AutoSize = true;
            this.sysStatFlowLayout_flowLayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.sysStatFlowLayout_flowLayoutPanel.Controls.Add(this.sysStatConn_checkBox);
            this.sysStatFlowLayout_flowLayoutPanel.Controls.Add(this.sysStatDisk_checkBox);
            this.sysStatFlowLayout_flowLayoutPanel.Controls.Add(this.sysStatTrans_checkBox);
            this.sysStatFlowLayout_flowLayoutPanel.Controls.Add(this.panel20);
            this.sysStatFlowLayout_flowLayoutPanel.Location = new System.Drawing.Point(3, 3);
            this.sysStatFlowLayout_flowLayoutPanel.Name = "sysStatFlowLayout_flowLayoutPanel";
            this.sysStatFlowLayout_flowLayoutPanel.Size = new System.Drawing.Size(329, 24);
            this.sysStatFlowLayout_flowLayoutPanel.TabIndex = 16;
            // 
            // sysStatConn_checkBox
            // 
            this.sysStatConn_checkBox.AutoSize = true;
            this.sysStatConn_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sysStatConn_checkBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sysStatConn_checkBox.Location = new System.Drawing.Point(3, 3);
            this.sysStatConn_checkBox.Name = "sysStatConn_checkBox";
            this.sysStatConn_checkBox.Size = new System.Drawing.Size(92, 18);
            this.sysStatConn_checkBox.TabIndex = 0;
            this.sysStatConn_checkBox.Text = "Connectivity";
            this.sysStatConn_checkBox.UseVisualStyleBackColor = true;
            // 
            // sysStatDisk_checkBox
            // 
            this.sysStatDisk_checkBox.AutoSize = true;
            this.sysStatDisk_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sysStatDisk_checkBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sysStatDisk_checkBox.Location = new System.Drawing.Point(101, 3);
            this.sysStatDisk_checkBox.Name = "sysStatDisk_checkBox";
            this.sysStatDisk_checkBox.Size = new System.Drawing.Size(56, 18);
            this.sysStatDisk_checkBox.TabIndex = 2;
            this.sysStatDisk_checkBox.Text = "Disks";
            this.sysStatDisk_checkBox.UseVisualStyleBackColor = true;
            // 
            // sysStatTrans_checkBox
            // 
            this.sysStatTrans_checkBox.AutoSize = true;
            this.sysStatTrans_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sysStatTrans_checkBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sysStatTrans_checkBox.Location = new System.Drawing.Point(163, 3);
            this.sysStatTrans_checkBox.Name = "sysStatTrans_checkBox";
            this.sysStatTrans_checkBox.Size = new System.Drawing.Size(93, 18);
            this.sysStatTrans_checkBox.TabIndex = 1;
            this.sysStatTrans_checkBox.Text = "Transactions";
            this.sysStatTrans_checkBox.UseVisualStyleBackColor = true;
            // 
            // panel20
            // 
            this.panel20.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel20.Controls.Add(this.oneGuiLabel8);
            this.panel20.Controls.Add(this.sysStatAlarm_checkbox);
            this.panel20.Location = new System.Drawing.Point(259, 0);
            this.panel20.Margin = new System.Windows.Forms.Padding(0);
            this.panel20.Name = "panel20";
            this.panel20.Size = new System.Drawing.Size(70, 22);
            this.panel20.TabIndex = 17;
            // 
            // oneGuiLabel8
            // 
            this.oneGuiLabel8.AutoSize = true;
            this.oneGuiLabel8.Font = new System.Drawing.Font("Tahoma", 8F);
            this.oneGuiLabel8.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel8.Location = new System.Drawing.Point(49, 1);
            this.oneGuiLabel8.Name = "oneGuiLabel8";
            this.oneGuiLabel8.Size = new System.Drawing.Size(13, 13);
            this.oneGuiLabel8.TabIndex = 14;
            this.oneGuiLabel8.Text = "*";
            // 
            // sysStatAlarm_checkbox
            // 
            this.sysStatAlarm_checkbox.AutoSize = true;
            this.sysStatAlarm_checkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sysStatAlarm_checkbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.sysStatAlarm_checkbox.Location = new System.Drawing.Point(2, 3);
            this.sysStatAlarm_checkbox.Name = "sysStatAlarm_checkbox";
            this.sysStatAlarm_checkbox.Size = new System.Drawing.Size(60, 18);
            this.sysStatAlarm_checkbox.TabIndex = 3;
            this.sysStatAlarm_checkbox.Text = global::Trafodion.Manager.OverviewArea.Properties.Resources.Alerts;
            this.sysStatAlarm_checkbox.UseVisualStyleBackColor = true;
            // 
            // sysStatRedText_oneGuiLabel
            // 
            this.sysStatRedText_oneGuiLabel.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.sysStatRedText_oneGuiLabel.AutoSize = true;
            this.sysStatRedText_oneGuiLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.sysStatRedText_oneGuiLabel.ForeColor = System.Drawing.Color.Red;
            this.sysStatRedText_oneGuiLabel.Location = new System.Drawing.Point(72, 30);
            this.sysStatRedText_oneGuiLabel.Name = "sysStatRedText_oneGuiLabel";
            this.sysStatRedText_oneGuiLabel.Size = new System.Drawing.Size(191, 13);
            this.sysStatRedText_oneGuiLabel.TabIndex = 15;
            this.sysStatRedText_oneGuiLabel.Text = "* Requires an active ODBC connection";
            // 
            // oneGuiGroupBox2
            // 
            this.oneGuiGroupBox2.AutoSize = true;
            this.oneGuiGroupBox2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.oneGuiGroupBox2.Controls.Add(this.sysStatTopBottom_flowLayoutPanel);
            this.oneGuiGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox2.Location = new System.Drawing.Point(3, 72);
            this.oneGuiGroupBox2.Name = "oneGuiGroupBox2";
            this.oneGuiGroupBox2.Size = new System.Drawing.Size(211, 44);
            this.oneGuiGroupBox2.TabIndex = 4;
            this.oneGuiGroupBox2.TabStop = false;
            this.oneGuiGroupBox2.Text = "System Status Location";
            // 
            // sysStatTopBottom_flowLayoutPanel
            // 
            this.sysStatTopBottom_flowLayoutPanel.AutoSize = true;
            this.sysStatTopBottom_flowLayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.sysStatTopBottom_flowLayoutPanel.Controls.Add(this.label1);
            this.sysStatTopBottom_flowLayoutPanel.Controls.Add(this.sysMonBottom_radioButton);
            this.sysStatTopBottom_flowLayoutPanel.Controls.Add(this.sysStatTop_radioButton);
            this.sysStatTopBottom_flowLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.sysStatTopBottom_flowLayoutPanel.Location = new System.Drawing.Point(3, 17);
            this.sysStatTopBottom_flowLayoutPanel.Name = "sysStatTopBottom_flowLayoutPanel";
            this.sysStatTopBottom_flowLayoutPanel.Size = new System.Drawing.Size(205, 24);
            this.sysStatTopBottom_flowLayoutPanel.TabIndex = 6;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(5, 5);
            this.label1.Margin = new System.Windows.Forms.Padding(5);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(69, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Display On:";
            // 
            // sysMonBottom_radioButton
            // 
            this.sysMonBottom_radioButton.AutoSize = true;
            this.sysMonBottom_radioButton.Checked = true;
            this.sysMonBottom_radioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sysMonBottom_radioButton.Location = new System.Drawing.Point(82, 3);
            this.sysMonBottom_radioButton.Name = "sysMonBottom_radioButton";
            this.sysMonBottom_radioButton.Size = new System.Drawing.Size(65, 18);
            this.sysMonBottom_radioButton.TabIndex = 4;
            this.sysMonBottom_radioButton.TabStop = true;
            this.sysMonBottom_radioButton.Text = "Bottom";
            this.sysMonBottom_radioButton.UseVisualStyleBackColor = true;
            // 
            // sysStatTop_radioButton
            // 
            this.sysStatTop_radioButton.AutoSize = true;
            this.sysStatTop_radioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.sysStatTop_radioButton.Location = new System.Drawing.Point(153, 3);
            this.sysStatTop_radioButton.Name = "sysStatTop_radioButton";
            this.sysStatTop_radioButton.Size = new System.Drawing.Size(49, 18);
            this.sysStatTop_radioButton.TabIndex = 3;
            this.sysStatTop_radioButton.Text = "Top";
            this.sysStatTop_radioButton.UseVisualStyleBackColor = true;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.AutoSize = true;
            this.flowLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel1.Controls.Add(this.groupBox3);
            this.flowLayoutPanel1.Controls.Add(this.groupBox12);
            this.flowLayoutPanel1.Controls.Add(this.groupBox6);
            this.flowLayoutPanel1.Controls.Add(this.gbTimelineOptions_groupBox);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(4, 609);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(560, 74);
            this.flowLayoutPanel1.TabIndex = 138;
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.miscAggSegs_radioButton);
            this.groupBox3.Controls.Add(this.miscAggNone_radioButton);
            this.groupBox3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox3.Location = new System.Drawing.Point(3, 3);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(124, 68);
            this.groupBox3.TabIndex = 111;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Aggregation Options";
            this.groupBox3.Visible = false;
            // 
            // miscAggSegs_radioButton
            // 
            this.miscAggSegs_radioButton.AutoSize = true;
            this.miscAggSegs_radioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.miscAggSegs_radioButton.Location = new System.Drawing.Point(18, 43);
            this.miscAggSegs_radioButton.Name = "miscAggSegs_radioButton";
            this.miscAggSegs_radioButton.Size = new System.Drawing.Size(81, 18);
            this.miscAggSegs_radioButton.TabIndex = 2;
            this.miscAggSegs_radioButton.TabStop = true;
            this.miscAggSegs_radioButton.Text = " Segments";
            this.miscAggSegs_radioButton.UseVisualStyleBackColor = true;
            // 
            // miscAggNone_radioButton
            // 
            this.miscAggNone_radioButton.AutoSize = true;
            this.miscAggNone_radioButton.Checked = true;
            this.miscAggNone_radioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.miscAggNone_radioButton.Location = new System.Drawing.Point(18, 21);
            this.miscAggNone_radioButton.Name = "miscAggNone_radioButton";
            this.miscAggNone_radioButton.Size = new System.Drawing.Size(106, 18);
            this.miscAggNone_radioButton.TabIndex = 0;
            this.miscAggNone_radioButton.TabStop = true;
            this.miscAggNone_radioButton.Text = "No Aggregation";
            this.miscAggNone_radioButton.UseVisualStyleBackColor = true;
            // 
            // groupBox12
            // 
            this.groupBox12.Controls.Add(this.miscToggleBG_checkBox);
            this.groupBox12.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox12.Location = new System.Drawing.Point(133, 3);
            this.groupBox12.Name = "groupBox12";
            this.groupBox12.Size = new System.Drawing.Size(124, 68);
            this.groupBox12.TabIndex = 136;
            this.groupBox12.TabStop = false;
            this.groupBox12.Text = "Timeout Color Toggle";
            // 
            // miscToggleBG_checkBox
            // 
            this.miscToggleBG_checkBox.AutoSize = true;
            this.miscToggleBG_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.miscToggleBG_checkBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.miscToggleBG_checkBox.Location = new System.Drawing.Point(16, 19);
            this.miscToggleBG_checkBox.Name = "miscToggleBG_checkBox";
            this.miscToggleBG_checkBox.Size = new System.Drawing.Size(109, 44);
            this.miscToggleBG_checkBox.TabIndex = 123;
            this.miscToggleBG_checkBox.Text = "Background \nChange on\nConnection Lost";
            this.miscToggleBG_checkBox.UseVisualStyleBackColor = true;
            // 
            // groupBox6
            // 
            this.groupBox6.Controls.Add(this.miscShowSep_checkBox);
            this.groupBox6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox6.Location = new System.Drawing.Point(263, 3);
            this.groupBox6.Name = "groupBox6";
            this.groupBox6.Size = new System.Drawing.Size(133, 68);
            this.groupBox6.TabIndex = 137;
            this.groupBox6.TabStop = false;
            this.groupBox6.Text = "Additional Settings";
            // 
            // miscShowSep_checkBox
            // 
            this.miscShowSep_checkBox.AutoSize = true;
            this.miscShowSep_checkBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.miscShowSep_checkBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.miscShowSep_checkBox.Location = new System.Drawing.Point(10, 32);
            this.miscShowSep_checkBox.Name = "miscShowSep_checkBox";
            this.miscShowSep_checkBox.Size = new System.Drawing.Size(114, 18);
            this.miscShowSep_checkBox.TabIndex = 0;
            this.miscShowSep_checkBox.Text = "Show Separators";
            this.miscShowSep_checkBox.UseVisualStyleBackColor = true;
            // 
            // gbTimelineOptions_groupBox
            // 
            this.gbTimelineOptions_groupBox.Controls.Add(this.miscTimelineMax_textBox);
            this.gbTimelineOptions_groupBox.Controls.Add(this.label19);
            this.gbTimelineOptions_groupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.gbTimelineOptions_groupBox.Location = new System.Drawing.Point(402, 3);
            this.gbTimelineOptions_groupBox.Name = "gbTimelineOptions_groupBox";
            this.gbTimelineOptions_groupBox.Size = new System.Drawing.Size(124, 68);
            this.gbTimelineOptions_groupBox.TabIndex = 110;
            this.gbTimelineOptions_groupBox.TabStop = false;
            this.gbTimelineOptions_groupBox.Text = "Timeline Options";
            // 
            // miscTimelineMax_textBox
            // 
            this.miscTimelineMax_textBox.Enabled = false;
            this.miscTimelineMax_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.miscTimelineMax_textBox.Location = new System.Drawing.Point(21, 38);
            this.miscTimelineMax_textBox.Name = "miscTimelineMax_textBox";
            this.miscTimelineMax_textBox.Size = new System.Drawing.Size(80, 21);
            this.miscTimelineMax_textBox.TabIndex = 2;
            this.miscTimelineMax_textBox.TextChanged += new System.EventHandler(this.textBox_PasteValidate);
            this.miscTimelineMax_textBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBox_KeyPressValidate);
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label19.Location = new System.Drawing.Point(22, 20);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(65, 13);
            this.label19.TabIndex = 3;
            this.label19.Text = "Max Range:";
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.AutoSize = true;
            this.flowLayoutPanel2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel2.Controls.Add(this.groupBox5);
            this.flowLayoutPanel2.Controls.Add(this.groupBox10);
            this.flowLayoutPanel2.Location = new System.Drawing.Point(4, 489);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(426, 113);
            this.flowLayoutPanel2.TabIndex = 139;
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.label21);
            this.groupBox5.Controls.Add(this.label20);
            this.groupBox5.Controls.Add(this.label22);
            this.groupBox5.Controls.Add(this.colBGBar_button);
            this.groupBox5.Controls.Add(this.colBGTimeline_button);
            this.groupBox5.Controls.Add(this.colBGDiscon_button);
            this.groupBox5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox5.Location = new System.Drawing.Point(3, 3);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(207, 107);
            this.groupBox5.TabIndex = 130;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Chart Background Options";
            // 
            // label21
            // 
            this.label21.AutoSize = true;
            this.label21.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label21.Location = new System.Drawing.Point(44, 51);
            this.label21.Name = "label21";
            this.label21.Size = new System.Drawing.Size(153, 13);
            this.label21.TabIndex = 121;
            this.label21.Text = "Bar Graph Background";
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label20.Location = new System.Drawing.Point(44, 24);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(144, 13);
            this.label20.TabIndex = 120;
            this.label20.Text = "Timeline Background";
            // 
            // label22
            // 
            this.label22.AutoSize = true;
            this.label22.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label22.Location = new System.Drawing.Point(44, 72);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(122, 26);
            this.label22.TabIndex = 125;
            this.label22.Text = "Lost Connection\nBackground Color";
            // 
            // colBGBar_button
            // 
            this.colBGBar_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colBGBar_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colBGBar_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colBGBar_button.Location = new System.Drawing.Point(11, 47);
            this.colBGBar_button.Name = "colBGBar_button";
            this.colBGBar_button.Size = new System.Drawing.Size(27, 21);
            this.colBGBar_button.TabIndex = 119;
            this.colBGBar_button.UseVisualStyleBackColor = true;
            this.colBGBar_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // colBGTimeline_button
            // 
            this.colBGTimeline_button.BackColor = System.Drawing.SystemColors.InactiveBorder;
            this.colBGTimeline_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colBGTimeline_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colBGTimeline_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colBGTimeline_button.Location = new System.Drawing.Point(11, 20);
            this.colBGTimeline_button.Name = "colBGTimeline_button";
            this.colBGTimeline_button.Size = new System.Drawing.Size(27, 21);
            this.colBGTimeline_button.TabIndex = 114;
            this.colBGTimeline_button.UseVisualStyleBackColor = false;
            this.colBGTimeline_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // colBGDiscon_button
            // 
            this.colBGDiscon_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colBGDiscon_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colBGDiscon_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colBGDiscon_button.Location = new System.Drawing.Point(11, 75);
            this.colBGDiscon_button.Name = "colBGDiscon_button";
            this.colBGDiscon_button.Size = new System.Drawing.Size(27, 21);
            this.colBGDiscon_button.TabIndex = 124;
            this.colBGDiscon_button.UseVisualStyleBackColor = false;
            this.colBGDiscon_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // groupBox10
            // 
            this.groupBox10.Controls.Add(this.colMiscMouse_button);
            this.groupBox10.Controls.Add(this.label32);
            this.groupBox10.Controls.Add(this.labelCPUDown);
            this.groupBox10.Controls.Add(this.label33);
            this.groupBox10.Controls.Add(this.colMiscCPUDown_button);
            this.groupBox10.Controls.Add(this.colMiscThreshExceeded_button);
            this.groupBox10.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox10.Location = new System.Drawing.Point(216, 3);
            this.groupBox10.Name = "groupBox10";
            this.groupBox10.Size = new System.Drawing.Size(207, 107);
            this.groupBox10.TabIndex = 133;
            this.groupBox10.TabStop = false;
            this.groupBox10.Text = "Additional Color Options";
            // 
            // colMiscMouse_button
            // 
            this.colMiscMouse_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colMiscMouse_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colMiscMouse_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colMiscMouse_button.Location = new System.Drawing.Point(8, 19);
            this.colMiscMouse_button.Name = "colMiscMouse_button";
            this.colMiscMouse_button.Size = new System.Drawing.Size(27, 21);
            this.colMiscMouse_button.TabIndex = 126;
            this.colMiscMouse_button.UseVisualStyleBackColor = true;
            this.colMiscMouse_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // label32
            // 
            this.label32.AutoSize = true;
            this.label32.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label32.Location = new System.Drawing.Point(41, 23);
            this.label32.Name = "label32";
            this.label32.Size = new System.Drawing.Size(149, 13);
            this.label32.TabIndex = 125;
            this.label32.Text = "Mouse-Over Bar Color";
            // 
            // labelCPUDown
            // 
            this.labelCPUDown.AutoSize = true;
            this.labelCPUDown.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelCPUDown.Location = new System.Drawing.Point(41, 79);
            this.labelCPUDown.Name = "labelCPUDown";
            this.labelCPUDown.Size = new System.Drawing.Size(135, 13);
            this.labelCPUDown.TabIndex = 125;
            this.labelCPUDown.Text = "CPU Down Bar Color";
            // 
            // label33
            // 
            this.label33.AutoSize = true;
            this.label33.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label33.Location = new System.Drawing.Point(41, 44);
            this.label33.Name = "label33";
            this.label33.Size = new System.Drawing.Size(139, 26);
            this.label33.TabIndex = 125;
            this.label33.Text = "Threshold Exceeded\n Bar Color";
            this.label33.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // colMiscCPUDown_button
            // 
            this.colMiscCPUDown_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colMiscCPUDown_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colMiscCPUDown_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colMiscCPUDown_button.ForeColor = System.Drawing.SystemColors.ActiveCaptionText;
            this.colMiscCPUDown_button.Location = new System.Drawing.Point(8, 74);
            this.colMiscCPUDown_button.Name = "colMiscCPUDown_button";
            this.colMiscCPUDown_button.Size = new System.Drawing.Size(27, 21);
            this.colMiscCPUDown_button.TabIndex = 126;
            this.colMiscCPUDown_button.UseVisualStyleBackColor = true;
            this.colMiscCPUDown_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // colMiscThreshExceeded_button
            // 
            this.colMiscThreshExceeded_button.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.colMiscThreshExceeded_button.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.colMiscThreshExceeded_button.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.colMiscThreshExceeded_button.Location = new System.Drawing.Point(8, 46);
            this.colMiscThreshExceeded_button.Name = "colMiscThreshExceeded_button";
            this.colMiscThreshExceeded_button.Size = new System.Drawing.Size(27, 21);
            this.colMiscThreshExceeded_button.TabIndex = 126;
            this.colMiscThreshExceeded_button.UseVisualStyleBackColor = true;
            this.colMiscThreshExceeded_button.Click += new System.EventHandler(this.ColorButton_Click);
            // 
            // flowLayoutPanel6
            // 
            this.flowLayoutPanel6.AutoSize = true;
            this.flowLayoutPanel6.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel6.Controls.Add(this.groupBox4);
            this.flowLayoutPanel6.Controls.Add(this.groupBox11);
            this.flowLayoutPanel6.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel6.Location = new System.Drawing.Point(4, 4);
            this.flowLayoutPanel6.Name = "flowLayoutPanel6";
            this.flowLayoutPanel6.Size = new System.Drawing.Size(560, 72);
            this.flowLayoutPanel6.TabIndex = 141;
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.portNumINC_oneGuiMaskedTextBox);
            this.groupBox4.Controls.Add(this.portNum_oneGuiMaskedTextBox);
            this.groupBox4.Controls.Add(this.portNumINC_Checkbox);
            this.groupBox4.Controls.Add(this.label2);
            this.groupBox4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox4.Location = new System.Drawing.Point(3, 3);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(237, 66);
            this.groupBox4.TabIndex = 112;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Port";
            // 
            // portNumINC_oneGuiMaskedTextBox
            // 
            this.portNumINC_oneGuiMaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.portNumINC_oneGuiMaskedTextBox.Location = new System.Drawing.Point(125, 40);
            this.portNumINC_oneGuiMaskedTextBox.Mask = "00000";
            this.portNumINC_oneGuiMaskedTextBox.Name = "portNumINC_oneGuiMaskedTextBox";
            this.portNumINC_oneGuiMaskedTextBox.PromptChar = ' ';
            this.portNumINC_oneGuiMaskedTextBox.Size = new System.Drawing.Size(72, 20);
            this.portNumINC_oneGuiMaskedTextBox.TabIndex = 8;
            this.portNumINC_oneGuiMaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.portNumINC_oneGuiMaskedTextBox.TextChanged += new System.EventHandler(this.portNumINC_oneGuiMaskedTextBox_TextChanged);
            // 
            // portNum_oneGuiMaskedTextBox
            // 
            this.portNum_oneGuiMaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.portNum_oneGuiMaskedTextBox.Location = new System.Drawing.Point(91, 15);
            this.portNum_oneGuiMaskedTextBox.Mask = "00000";
            this.portNum_oneGuiMaskedTextBox.Name = "portNum_oneGuiMaskedTextBox";
            this.portNum_oneGuiMaskedTextBox.PromptChar = ' ';
            this.portNum_oneGuiMaskedTextBox.Size = new System.Drawing.Size(72, 20);
            this.portNum_oneGuiMaskedTextBox.TabIndex = 7;
            this.portNum_oneGuiMaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.portNum_oneGuiMaskedTextBox.TextChanged += new System.EventHandler(this.portNum_oneGuiMaskedTextBox_TextChanged);
            // 
            // portNumINC_Checkbox
            // 
            this.portNumINC_Checkbox.AutoSize = true;
            this.portNumINC_Checkbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.portNumINC_Checkbox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.portNumINC_Checkbox.Location = new System.Drawing.Point(13, 40);
            this.portNumINC_Checkbox.Name = "portNumINC_Checkbox";
            this.portNumINC_Checkbox.Size = new System.Drawing.Size(121, 18);
            this.portNumINC_Checkbox.TabIndex = 5;
            this.portNumINC_Checkbox.Text = "Fixed Incoming:";
            this.portNumINC_Checkbox.UseVisualStyleBackColor = true;
            this.portNumINC_Checkbox.CheckedChanged += new System.EventHandler(this.portNumINC_Checkbox_CheckedChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(11, 19);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(81, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Port Number:";
            // 
            // groupBox11
            // 
            this.groupBox11.Controls.Add(this.refreshRate_MaskedTextBox);
            this.groupBox11.Controls.Add(this._lblSeconds);
            this.groupBox11.Controls.Add(this.label35);
            this.groupBox11.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.groupBox11.Location = new System.Drawing.Point(246, 3);
            this.groupBox11.Name = "groupBox11";
            this.groupBox11.Size = new System.Drawing.Size(248, 66);
            this.groupBox11.TabIndex = 111;
            this.groupBox11.TabStop = false;
            this.groupBox11.Text = "Refresh Rate Options";
            // 
            // refreshRate_MaskedTextBox
            // 
            this.refreshRate_MaskedTextBox.Font = new System.Drawing.Font("Arial", 8F);
            this.refreshRate_MaskedTextBox.Location = new System.Drawing.Point(97, 24);
            this.refreshRate_MaskedTextBox.Mask = "000";
            this.refreshRate_MaskedTextBox.Name = "refreshRate_MaskedTextBox";
            this.refreshRate_MaskedTextBox.PromptChar = ' ';
            this.refreshRate_MaskedTextBox.Size = new System.Drawing.Size(60, 20);
            this.refreshRate_MaskedTextBox.TabIndex = 8;
            this.refreshRate_MaskedTextBox.Enter += new System.EventHandler(this.TrafodionMaskedTextBox_Enter);
            this.refreshRate_MaskedTextBox.TextChanged += new System.EventHandler(this.refreshRate_MaskedTextBox_TextChanged);
            // 
            // _lblSeconds
            // 
            this._lblSeconds.AutoSize = true;
            this._lblSeconds.Location = new System.Drawing.Point(160, 27);
            this._lblSeconds.Name = "_lblSeconds";
            this._lblSeconds.Size = new System.Drawing.Size(47, 13);
            this._lblSeconds.TabIndex = 4;
            this._lblSeconds.Text = "Seconds";
            // 
            // label35
            // 
            this.label35.AutoSize = true;
            this.label35.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label35.Location = new System.Drawing.Point(11, 27);
            this.label35.Name = "label35";
            this.label35.Size = new System.Drawing.Size(84, 13);
            this.label35.TabIndex = 3;
            this.label35.Text = "Refresh Rate:";
            // 
            // maxValCPUBSY_textBox
            // 
            this.maxValCPUBSY_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxValCPUBSY_textBox.Location = new System.Drawing.Point(0, 0);
            this.maxValCPUBSY_textBox.Name = "maxValCPUBSY_textBox";
            this.maxValCPUBSY_textBox.Size = new System.Drawing.Size(100, 21);
            this.maxValCPUBSY_textBox.TabIndex = 0;
            // 
            // maxValDiskIO_textBox
            // 
            this.maxValDiskIO_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxValDiskIO_textBox.Location = new System.Drawing.Point(0, 0);
            this.maxValDiskIO_textBox.Name = "maxValDiskIO_textBox";
            this.maxValDiskIO_textBox.Size = new System.Drawing.Size(100, 21);
            this.maxValDiskIO_textBox.TabIndex = 0;
            // 
            // maxCache_textBox
            // 
            this.maxCache_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxCache_textBox.Location = new System.Drawing.Point(0, 0);
            this.maxCache_textBox.Name = "maxCache_textBox";
            this.maxCache_textBox.Size = new System.Drawing.Size(100, 21);
            this.maxCache_textBox.TabIndex = 0;
            // 
            // maxDispatch_textBox
            // 
            this.maxDispatch_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxDispatch_textBox.Location = new System.Drawing.Point(0, 0);
            this.maxDispatch_textBox.Name = "maxDispatch_textBox";
            this.maxDispatch_textBox.Size = new System.Drawing.Size(100, 21);
            this.maxDispatch_textBox.TabIndex = 0;
            // 
            // maxValSwap_textBox
            // 
            this.maxValSwap_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxValSwap_textBox.Location = new System.Drawing.Point(0, 0);
            this.maxValSwap_textBox.Name = "maxValSwap_textBox";
            this.maxValSwap_textBox.Size = new System.Drawing.Size(100, 21);
            this.maxValSwap_textBox.TabIndex = 0;
            // 
            // maxValFreeMem_TextBox
            // 
            this.maxValFreeMem_TextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxValFreeMem_TextBox.Location = new System.Drawing.Point(0, 0);
            this.maxValFreeMem_TextBox.Name = "maxValFreeMem_TextBox";
            this.maxValFreeMem_TextBox.Size = new System.Drawing.Size(100, 21);
            this.maxValFreeMem_TextBox.TabIndex = 0;
            // 
            // maxCPUQL_textBox
            // 
            this.maxCPUQL_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.maxCPUQL_textBox.Location = new System.Drawing.Point(0, 0);
            this.maxCPUQL_textBox.Name = "maxCPUQL_textBox";
            this.maxCPUQL_textBox.Size = new System.Drawing.Size(100, 21);
            this.maxCPUQL_textBox.TabIndex = 0;
            // 
            // refreshRate_textBox
            // 
            this.refreshRate_textBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshRate_textBox.Location = new System.Drawing.Point(0, 0);
            this.refreshRate_textBox.Name = "refreshRate_textBox";
            this.refreshRate_textBox.Size = new System.Drawing.Size(100, 21);
            this.refreshRate_textBox.TabIndex = 0;
            // 
            // label5
            // 
            this.label5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label5.Location = new System.Drawing.Point(0, 0);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(100, 23);
            this.label5.TabIndex = 0;
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.oneGuiPanel1.AutoSize = true;
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.label34);
            this.oneGuiPanel1.Controls.Add(this.radioButton2);
            this.oneGuiPanel1.Controls.Add(this.radioButton5);
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 52);
            this.oneGuiPanel1.Margin = new System.Windows.Forms.Padding(0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(192, 26);
            this.oneGuiPanel1.TabIndex = 1;
            // 
            // label34
            // 
            this.label34.AutoSize = true;
            this.label34.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label34.Location = new System.Drawing.Point(4, 7);
            this.label34.Name = "label34";
            this.label34.Size = new System.Drawing.Size(69, 13);
            this.label34.TabIndex = 5;
            this.label34.Text = "Display On:";
            // 
            // radioButton2
            // 
            this.radioButton2.AutoSize = true;
            this.radioButton2.Checked = true;
            this.radioButton2.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.radioButton2.Location = new System.Drawing.Point(81, 6);
            this.radioButton2.Name = "radioButton2";
            this.radioButton2.Size = new System.Drawing.Size(50, 18);
            this.radioButton2.TabIndex = 3;
            this.radioButton2.TabStop = true;
            this.radioButton2.Text = "Top";
            this.radioButton2.UseVisualStyleBackColor = true;
            // 
            // radioButton5
            // 
            this.radioButton5.AutoSize = true;
            this.radioButton5.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.radioButton5.Location = new System.Drawing.Point(131, 6);
            this.radioButton5.Name = "radioButton5";
            this.radioButton5.Size = new System.Drawing.Size(64, 18);
            this.radioButton5.TabIndex = 4;
            this.radioButton5.Text = "Bottom";
            this.radioButton5.UseVisualStyleBackColor = true;
            // 
            // errorProvider1
            // 
            this.errorProvider1.ContainerControl = this;
            // 
            // SystemMonitorConfigurationControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.scrollPanel_oneGuiPanel);
            this.Controls.Add(this.panel35);
            this.Name = "SystemMonitorConfigurationControl";
            this.Size = new System.Drawing.Size(587, 819);
            this.panel35.ResumeLayout(false);
            this.panel35.PerformLayout();
            this.flowLayoutPanel13.ResumeLayout(false);
            this.tableLayoutPanel2.ResumeLayout(false);
            this.tableLayoutPanel2.PerformLayout();
            this.flowLayoutPanel5.ResumeLayout(false);
            this.flowLayoutPanel5.PerformLayout();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.panel9.ResumeLayout(false);
            this.panel9.PerformLayout();
            this.panel8.ResumeLayout(false);
            this.panel8.PerformLayout();
            this.panel7.ResumeLayout(false);
            this.panel7.PerformLayout();
            this.scrollPanel_oneGuiPanel.ResumeLayout(false);
            this.scrollPanel_oneGuiPanel.PerformLayout();
            this.nsmTable_tableLayoutPanel.ResumeLayout(false);
            this.nsmTable_tableLayoutPanel.PerformLayout();
            this.tableLayoutPanel4.ResumeLayout(false);
            this.tableLayoutPanel4.PerformLayout();
            this.flowLayoutPanel4.ResumeLayout(false);
            this.panel5.ResumeLayout(false);
            this.panel5.PerformLayout();
            this.panel13.ResumeLayout(false);
            this.panel13.PerformLayout();
            this.flowLayoutPanel7.ResumeLayout(false);
            this.panel6.ResumeLayout(false);
            this.panel6.PerformLayout();
            this.panel14.ResumeLayout(false);
            this.panel14.PerformLayout();
            this.flowLayoutPanel8.ResumeLayout(false);
            this.panel11.ResumeLayout(false);
            this.panel11.PerformLayout();
            this.panel15.ResumeLayout(false);
            this.panel15.PerformLayout();
            this.flowLayoutPanel9.ResumeLayout(false);
            this.panel12.ResumeLayout(false);
            this.panel12.PerformLayout();
            this.panel16.ResumeLayout(false);
            this.panel16.PerformLayout();
            this.flowLayoutPanel10.ResumeLayout(false);
            this.panel3.ResumeLayout(false);
            this.panel3.PerformLayout();
            this.panel17.ResumeLayout(false);
            this.panel17.PerformLayout();
            this.flowLayoutPanel11.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.panel18.ResumeLayout(false);
            this.panel18.PerformLayout();
            this.flowLayoutPanel12.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.panel19.ResumeLayout(false);
            this.panel19.PerformLayout();
            this.flowLayoutPanel14.ResumeLayout(false);
            this.flowLayoutPanel14.PerformLayout();
            this.panel10.ResumeLayout(false);
            this.panel10.PerformLayout();
            this.flowLayoutPanel3.ResumeLayout(false);
            this.flowLayoutPanel3.PerformLayout();
            this.oneGuiGroupBox1.ResumeLayout(false);
            this.oneGuiGroupBox1.PerformLayout();
            this.flowLayoutPanel15.ResumeLayout(false);
            this.flowLayoutPanel15.PerformLayout();
            this.sysStatFlowLayout_flowLayoutPanel.ResumeLayout(false);
            this.sysStatFlowLayout_flowLayoutPanel.PerformLayout();
            this.panel20.ResumeLayout(false);
            this.panel20.PerformLayout();
            this.oneGuiGroupBox2.ResumeLayout(false);
            this.oneGuiGroupBox2.PerformLayout();
            this.sysStatTopBottom_flowLayoutPanel.ResumeLayout(false);
            this.sysStatTopBottom_flowLayoutPanel.PerformLayout();
            this.flowLayoutPanel1.ResumeLayout(false);
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox12.ResumeLayout(false);
            this.groupBox12.PerformLayout();
            this.groupBox6.ResumeLayout(false);
            this.groupBox6.PerformLayout();
            this.gbTimelineOptions_groupBox.ResumeLayout(false);
            this.gbTimelineOptions_groupBox.PerformLayout();
            this.flowLayoutPanel2.ResumeLayout(false);
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox10.ResumeLayout(false);
            this.groupBox10.PerformLayout();
            this.flowLayoutPanel6.ResumeLayout(false);
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.groupBox11.ResumeLayout(false);
            this.groupBox11.PerformLayout();
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }





        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _systemMonitorToolTip;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel35;
        private Trafodion.Manager.Framework.Controls.TrafodionButton button25;
        private TrafodionButton applyConfig_oneGuiButton;
        public System.Windows.Forms.TableLayoutPanel nsmTable_tableLayoutPanel;
        public TrafodionPanel scrollPanel_oneGuiPanel;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel5;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox checkBox24;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox checkBox25;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox checkBox26;
        private TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label34;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton radioButton2;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton radioButton5;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel3;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton sysStatTop_radioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton sysMonBottom_radioButton;
        private TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox sysStatConn_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox sysStatTrans_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox sysStatDisk_checkBox;
        private TrafodionGroupBox oneGuiGroupBox2;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel9;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colorCpuQueue_ColButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxCPUQueue_textBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox checkCpuQueue_Checkbox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel8;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colorFreeMem_ColButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxFreeMem_textBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox checkBox6;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel7;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colorSwap_ColButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxSwap_textBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox checkBox5;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel6;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox4;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label2;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox11;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox refreshRate_textBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label5;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label35;
        private System.Windows.Forms.Label _lblSeconds;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel4;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel10;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label4;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label3;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel4;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel4;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel5;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colCPUBSY_ColButton;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox metricCPUBSY_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxValCPUBSY_textBox;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel7;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel6;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colDiskIO_ColButton;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox metricDiskIO_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxValDiskIO_textBox;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel8;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel11;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colCacheHits_ColButton;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox metricCache_checkBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxCache_textBox;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel9;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel12;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colDispatch_Button;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox metricDispatch_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxDispatch_textBox;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox3;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton miscAggSegs_radioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton miscAggNone_radioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox12;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox miscToggleBG_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox6;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox miscShowSep_checkBox;
        public Trafodion.Manager.Framework.Controls.TrafodionGroupBox gbTimelineOptions_groupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox miscTimelineMax_textBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label19;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox5;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label21;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label20;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label22;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colBGBar_button;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colBGTimeline_button;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colBGDiscon_button;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox groupBox10;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colMiscMouse_button;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label32;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel labelCPUDown;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label33;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colMiscCPUDown_button;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colMiscThreshExceeded_button;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel10;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel3;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colSwap_button;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox metricSwap_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxValSwap_textBox;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel11;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colFreeMem_button;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox metricFreeMem_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxValFreeMem_TextBox;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel12;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton colCPUQL_button;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox metricCPUQL_checkBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox maxCPUQL_textBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox sysStatAlarm_checkbox;
        private TrafodionLabel sysStatRedText_oneGuiLabel;
        private TrafodionLabel oneGuiLabel8;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel13;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel14;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel13;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel14;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel15;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel16;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel17;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel18;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel19;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel20;
        private System.Windows.Forms.FlowLayoutPanel sysStatFlowLayout_flowLayoutPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel15;
        private System.Windows.Forms.FlowLayoutPanel sysStatTopBottom_flowLayoutPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox portNumINC_Checkbox;
        private TrafodionMaskedTextBox portNum_oneGuiMaskedTextBox;
        private System.Windows.Forms.ErrorProvider errorProvider1;
        private TrafodionMaskedTextBox refreshRate_MaskedTextBox;
        private TrafodionMaskedTextBox portNumINC_oneGuiMaskedTextBox;
        private TrafodionMaskedTextBox maxValCPUBSY_MaskedTextBox;
        private TrafodionMaskedTextBox maxValDiskIO_MaskedTextBox;
        private TrafodionMaskedTextBox maxCache_MaskedTextBox;
        private TrafodionMaskedTextBox maxDispatch_MaskedTextBox;
        private TrafodionMaskedTextBox maxValSwap_MaskedTextBox;
        private TrafodionMaskedTextBox maxValFreeMem_MaskedTextBox;
        private TrafodionMaskedTextBox maxCPUQL_MaskedTextBox;
    }
}
