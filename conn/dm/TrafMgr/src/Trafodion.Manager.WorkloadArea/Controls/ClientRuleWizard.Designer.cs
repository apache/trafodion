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
    partial class ClientRuleWizard
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ClientRuleWizard));
            this.clientRulesButtonPanel = new System.Windows.Forms.Panel();
            this.btnSaveRule = new System.Windows.Forms.Button();
            this.wizardCancel = new System.Windows.Forms.Button();
            this.wizardFinish = new System.Windows.Forms.Button();
            this.buttonComments = new System.Windows.Forms.Button();
            this.enableRuleLabel = new System.Windows.Forms.Label();
            this.checkBox_RuleEnabled = new System.Windows.Forms.CheckBox();
            this.ruleNameLabel = new System.Windows.Forms.Label();
            this.textBox_QueryName = new System.Windows.Forms.TextBox();
            this.ruleText = new System.Windows.Forms.Panel();
            this.ruleDescriptionPanel = new System.Windows.Forms.Panel();
            this.textBoxExpression = new System.Windows.Forms.RichTextBox();
            this.clientRuleWizardSelectionPanel = new System.Windows.Forms.Panel();
            this.clientRuleWizardTabControl = new System.Windows.Forms.TabControl();
            this.tabPage_Systems = new System.Windows.Forms.TabPage();
            this.panel5 = new System.Windows.Forms.Panel();
            this.checkBox_allSystem = new System.Windows.Forms.CheckBox();
            this.checkBox_allConnect = new System.Windows.Forms.CheckBox();
            this.treeView_Systems = new System.Windows.Forms.TreeView();
            this.clientRuleConditionsTabPage = new System.Windows.Forms.TabPage();
            this.ruleConditionsTreeView = new System.Windows.Forms.TreeView();
            this.clientRuleActionsTabPage = new System.Windows.Forms.TabPage();
            this.listView2 = new System.Windows.Forms.ListView();
            this.columnHeader3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.label3 = new System.Windows.Forms.Label();
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.buttonAdvancedMode = new System.Windows.Forms.Button();
            this.clientRuleWizardDescriptionPanel = new System.Windows.Forms.Panel();
            this.panelComments = new System.Windows.Forms.Panel();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.textAdditionalComments = new System.Windows.Forms.RichTextBox();
            this.textName = new System.Windows.Forms.TextBox();
            this.textPlatform = new System.Windows.Forms.TextBox();
            this.textEmail = new System.Windows.Forms.TextBox();
            this.clientRuleWizardHelpProvider = new System.Windows.Forms.HelpProvider();
            this.clientQueryRuleBindingSource = new System.Windows.Forms.BindingSource(this.components);
            this.helpButton = new System.Windows.Forms.Button();
            this.clientRulesButtonPanel.SuspendLayout();
            this.ruleDescriptionPanel.SuspendLayout();
            this.clientRuleWizardSelectionPanel.SuspendLayout();
            this.clientRuleWizardTabControl.SuspendLayout();
            this.tabPage_Systems.SuspendLayout();
            this.panel5.SuspendLayout();
            this.clientRuleConditionsTabPage.SuspendLayout();
            this.clientRuleActionsTabPage.SuspendLayout();
            this.clientRuleWizardDescriptionPanel.SuspendLayout();
            this.panelComments.SuspendLayout();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.clientQueryRuleBindingSource)).BeginInit();
            this.SuspendLayout();
            // 
            // clientRulesButtonPanel
            // 
            this.clientRulesButtonPanel.Controls.Add(this.btnSaveRule);
            this.clientRulesButtonPanel.Controls.Add(this.wizardCancel);
            this.clientRulesButtonPanel.Controls.Add(this.helpButton);
            this.clientRulesButtonPanel.Controls.Add(this.wizardFinish);
            this.clientRulesButtonPanel.Controls.Add(this.buttonComments);
            this.clientRulesButtonPanel.Controls.Add(this.enableRuleLabel);
            this.clientRulesButtonPanel.Controls.Add(this.checkBox_RuleEnabled);
            this.clientRulesButtonPanel.Controls.Add(this.ruleNameLabel);
            this.clientRulesButtonPanel.Controls.Add(this.textBox_QueryName);
            this.clientRulesButtonPanel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.clientRulesButtonPanel.Location = new System.Drawing.Point(281, 228);
            this.clientRulesButtonPanel.Name = "clientRulesButtonPanel";
            this.clientRulesButtonPanel.Size = new System.Drawing.Size(400, 147);
            this.clientRulesButtonPanel.TabIndex = 0;
            // 
            // btnSaveRule
            // 
            this.btnSaveRule.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSaveRule.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnSaveRule.Image = ((System.Drawing.Image)(resources.GetObject("btnSaveRule.Image")));
            this.btnSaveRule.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.btnSaveRule.Location = new System.Drawing.Point(12, 109);
            this.btnSaveRule.Name = "btnSaveRule";
            this.btnSaveRule.Size = new System.Drawing.Size(80, 25);
            this.btnSaveRule.TabIndex = 12;
            this.btnSaveRule.Text = "  Save";
            this.btnSaveRule.UseVisualStyleBackColor = true;
            this.btnSaveRule.Click += new System.EventHandler(this.btnSaveRule_Click);
            // 
            // wizardCancel
            // 
            this.wizardCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.wizardCancel.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.wizardCancel.Image = ((System.Drawing.Image)(resources.GetObject("wizardCancel.Image")));
            this.wizardCancel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.wizardCancel.Location = new System.Drawing.Point(132, 109);
            this.wizardCancel.Name = "wizardCancel";
            this.wizardCancel.Size = new System.Drawing.Size(80, 25);
            this.wizardCancel.TabIndex = 5;
            this.wizardCancel.Text = "    Cancel";
            this.wizardCancel.UseVisualStyleBackColor = true;
            this.wizardCancel.Click += new System.EventHandler(this.wizardCancel_Click);
            // 
            // wizardFinish
            // 
            this.wizardFinish.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.wizardFinish.Image = ((System.Drawing.Image)(resources.GetObject("wizardFinish.Image")));
            this.wizardFinish.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.wizardFinish.Location = new System.Drawing.Point(222, 109);
            this.wizardFinish.Name = "wizardFinish";
            this.wizardFinish.Size = new System.Drawing.Size(80, 25);
            this.wizardFinish.TabIndex = 2;
            this.wizardFinish.Text = "  Finish";
            this.wizardFinish.UseVisualStyleBackColor = true;
            this.wizardFinish.Click += new System.EventHandler(this.wizardFinish_Click);
            // 
            // buttonComments
            // 
            this.buttonComments.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.buttonComments.Location = new System.Drawing.Point(280, 25);
            this.buttonComments.Name = "buttonComments";
            this.buttonComments.Size = new System.Drawing.Size(80, 25);
            this.buttonComments.TabIndex = 11;
            this.buttonComments.Text = "Details ...";
            this.buttonComments.UseVisualStyleBackColor = true;
            this.buttonComments.Click += new System.EventHandler(this.buttonComments_Click);
            // 
            // enableRuleLabel
            // 
            this.enableRuleLabel.AutoSize = true;
            this.enableRuleLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.enableRuleLabel.Location = new System.Drawing.Point(10, 56);
            this.enableRuleLabel.Name = "enableRuleLabel";
            this.enableRuleLabel.Size = new System.Drawing.Size(113, 13);
            this.enableRuleLabel.TabIndex = 8;
            this.enableRuleLabel.Text = "Configure this rule:";
            // 
            // checkBox_RuleEnabled
            // 
            this.checkBox_RuleEnabled.AutoSize = true;
            this.checkBox_RuleEnabled.Checked = true;
            this.checkBox_RuleEnabled.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox_RuleEnabled.Location = new System.Drawing.Point(14, 72);
            this.checkBox_RuleEnabled.Name = "checkBox_RuleEnabled";
            this.checkBox_RuleEnabled.Size = new System.Drawing.Size(103, 20);
            this.checkBox_RuleEnabled.TabIndex = 6;
            this.checkBox_RuleEnabled.Text = "Enable this rule";
            this.checkBox_RuleEnabled.UseVisualStyleBackColor = true;
            // 
            // ruleNameLabel
            // 
            this.ruleNameLabel.AutoSize = true;
            this.ruleNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ruleNameLabel.Location = new System.Drawing.Point(10, 8);
            this.ruleNameLabel.Name = "ruleNameLabel";
            this.ruleNameLabel.Size = new System.Drawing.Size(164, 13);
            this.ruleNameLabel.TabIndex = 5;
            this.ruleNameLabel.Text = "Specify a name for this rule:";
            // 
            // textBox_QueryName
            // 
            this.textBox_QueryName.Font = new System.Drawing.Font("Verdana", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textBox_QueryName.Location = new System.Drawing.Point(10, 26);
            this.textBox_QueryName.Name = "textBox_QueryName";
            this.textBox_QueryName.Size = new System.Drawing.Size(260, 23);
            this.textBox_QueryName.TabIndex = 4;
            // 
            // ruleText
            // 
            this.ruleText.AutoScroll = true;
            this.ruleText.BackColor = System.Drawing.Color.White;
            this.ruleText.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.ruleText.Location = new System.Drawing.Point(0, 0);
            this.ruleText.Name = "ruleText";
            this.ruleText.Size = new System.Drawing.Size(395, 190);
            this.ruleText.TabIndex = 1;
            // 
            // ruleDescriptionPanel
            // 
            this.ruleDescriptionPanel.Controls.Add(this.ruleText);
            this.ruleDescriptionPanel.Controls.Add(this.textBoxExpression);
            this.ruleDescriptionPanel.Location = new System.Drawing.Point(5, 28);
            this.ruleDescriptionPanel.Name = "ruleDescriptionPanel";
            this.ruleDescriptionPanel.Size = new System.Drawing.Size(395, 190);
            this.ruleDescriptionPanel.TabIndex = 6;
            // 
            // textBoxExpression
            // 
            this.textBoxExpression.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.textBoxExpression.Location = new System.Drawing.Point(0, 0);
            this.textBoxExpression.Name = "textBoxExpression";
            this.textBoxExpression.Size = new System.Drawing.Size(395, 106);
            this.textBoxExpression.TabIndex = 10;
            this.textBoxExpression.Text = "";
            // 
            // clientRuleWizardSelectionPanel
            // 
            this.clientRuleWizardSelectionPanel.Controls.Add(this.clientRuleWizardTabControl);
            this.clientRuleWizardSelectionPanel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.clientRuleWizardSelectionPanel.Location = new System.Drawing.Point(4, 4);
            this.clientRuleWizardSelectionPanel.Name = "clientRuleWizardSelectionPanel";
            this.clientRuleWizardSelectionPanel.Size = new System.Drawing.Size(273, 375);
            this.clientRuleWizardSelectionPanel.TabIndex = 7;
            // 
            // clientRuleWizardTabControl
            // 
            this.clientRuleWizardTabControl.Controls.Add(this.tabPage_Systems);
            this.clientRuleWizardTabControl.Controls.Add(this.clientRuleConditionsTabPage);
            this.clientRuleWizardTabControl.Controls.Add(this.clientRuleActionsTabPage);
            this.clientRuleWizardTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.clientRuleWizardTabControl.Location = new System.Drawing.Point(0, 0);
            this.clientRuleWizardTabControl.Name = "clientRuleWizardTabControl";
            this.clientRuleWizardTabControl.SelectedIndex = 0;
            this.clientRuleWizardTabControl.Size = new System.Drawing.Size(273, 375);
            this.clientRuleWizardTabControl.TabIndex = 1;
            // 
            // tabPage_Systems
            // 
            this.tabPage_Systems.Controls.Add(this.panel5);
            this.tabPage_Systems.Controls.Add(this.treeView_Systems);
            this.tabPage_Systems.Location = new System.Drawing.Point(4, 25);
            this.tabPage_Systems.Name = "tabPage_Systems";
            this.tabPage_Systems.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage_Systems.Size = new System.Drawing.Size(265, 346);
            this.tabPage_Systems.TabIndex = 2;
            this.tabPage_Systems.Text = "Systems";
            this.tabPage_Systems.UseVisualStyleBackColor = true;
            // 
            // panel5
            // 
            this.panel5.BackColor = System.Drawing.Color.White;
            this.panel5.Controls.Add(this.checkBox_allSystem);
            this.panel5.Controls.Add(this.checkBox_allConnect);
            this.panel5.Location = new System.Drawing.Point(3, 4);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(263, 52);
            this.panel5.TabIndex = 5;
            // 
            // checkBox_allSystem
            // 
            this.checkBox_allSystem.AutoSize = true;
            this.checkBox_allSystem.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox_allSystem.Location = new System.Drawing.Point(3, 8);
            this.checkBox_allSystem.Name = "checkBox_allSystem";
            this.checkBox_allSystem.Size = new System.Drawing.Size(87, 18);
            this.checkBox_allSystem.TabIndex = 4;
            this.checkBox_allSystem.Text = "All Systems";
            this.checkBox_allSystem.UseVisualStyleBackColor = true;
            this.checkBox_allSystem.Click += new System.EventHandler(this.checkBox_allSystem_Click);
            // 
            // checkBox_allConnect
            // 
            this.checkBox_allConnect.AutoSize = true;
            this.checkBox_allConnect.BackColor = System.Drawing.Color.Transparent;
            this.checkBox_allConnect.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox_allConnect.Location = new System.Drawing.Point(3, 29);
            this.checkBox_allConnect.Name = "checkBox_allConnect";
            this.checkBox_allConnect.Size = new System.Drawing.Size(202, 18);
            this.checkBox_allConnect.TabIndex = 3;
            this.checkBox_allConnect.Text = "All currently connected Systems";
            this.checkBox_allConnect.UseVisualStyleBackColor = false;
            this.checkBox_allConnect.Click += new System.EventHandler(this.checkBox_allConnect_Click);
            // 
            // treeView_Systems
            // 
            this.treeView_Systems.CheckBoxes = true;
            this.treeView_Systems.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.treeView_Systems.Location = new System.Drawing.Point(3, 59);
            this.treeView_Systems.Name = "treeView_Systems";
            this.treeView_Systems.ShowLines = false;
            this.treeView_Systems.ShowPlusMinus = false;
            this.treeView_Systems.ShowRootLines = false;
            this.treeView_Systems.Size = new System.Drawing.Size(264, 284);
            this.treeView_Systems.TabIndex = 2;
            // 
            // clientRuleConditionsTabPage
            // 
            this.clientRuleConditionsTabPage.Controls.Add(this.ruleConditionsTreeView);
            this.clientRuleConditionsTabPage.Location = new System.Drawing.Point(4, 25);
            this.clientRuleConditionsTabPage.Name = "clientRuleConditionsTabPage";
            this.clientRuleConditionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.clientRuleConditionsTabPage.Size = new System.Drawing.Size(265, 346);
            this.clientRuleConditionsTabPage.TabIndex = 0;
            this.clientRuleConditionsTabPage.Text = "Conditions";
            this.clientRuleConditionsTabPage.UseVisualStyleBackColor = true;
            // 
            // ruleConditionsTreeView
            // 
            this.ruleConditionsTreeView.CheckBoxes = true;
            this.ruleConditionsTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ruleConditionsTreeView.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ruleConditionsTreeView.Location = new System.Drawing.Point(3, 3);
            this.ruleConditionsTreeView.Name = "ruleConditionsTreeView";
            this.ruleConditionsTreeView.Size = new System.Drawing.Size(259, 340);
            this.ruleConditionsTreeView.TabIndex = 1;
            // 
            // clientRuleActionsTabPage
            // 
            this.clientRuleActionsTabPage.Controls.Add(this.listView2);
            this.clientRuleActionsTabPage.Location = new System.Drawing.Point(4, 25);
            this.clientRuleActionsTabPage.Name = "clientRuleActionsTabPage";
            this.clientRuleActionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.clientRuleActionsTabPage.Size = new System.Drawing.Size(265, 346);
            this.clientRuleActionsTabPage.TabIndex = 1;
            this.clientRuleActionsTabPage.Text = "Actions";
            this.clientRuleActionsTabPage.UseVisualStyleBackColor = true;
            // 
            // listView2
            // 
            this.listView2.CheckBoxes = true;
            this.listView2.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader3});
            this.listView2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listView2.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.listView2.FullRowSelect = true;
            this.listView2.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.listView2.Location = new System.Drawing.Point(3, 3);
            this.listView2.Name = "listView2";
            this.listView2.Size = new System.Drawing.Size(259, 340);
            this.listView2.TabIndex = 0;
            this.listView2.UseCompatibleStateImageBehavior = false;
            this.listView2.View = System.Windows.Forms.View.Details;
            this.listView2.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.listView2_ItemCheck);
            // 
            // columnHeader3
            // 
            this.columnHeader3.Width = 200;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(3, 10);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(101, 13);
            this.label3.TabIndex = 8;
            this.label3.Text = "Rule description:";
            // 
            // columnHeader2
            // 
            this.columnHeader2.Width = 400;
            // 
            // buttonAdvancedMode
            // 
            this.buttonAdvancedMode.Location = new System.Drawing.Point(291, 5);
            this.buttonAdvancedMode.Name = "buttonAdvancedMode";
            this.buttonAdvancedMode.Size = new System.Drawing.Size(110, 27);
            this.buttonAdvancedMode.TabIndex = 9;
            this.buttonAdvancedMode.Text = "Advanced Mode";
            this.buttonAdvancedMode.UseVisualStyleBackColor = true;
            this.buttonAdvancedMode.Click += new System.EventHandler(this.buttonAdvancedMode_Click);
            // 
            // clientRuleWizardDescriptionPanel
            // 
            this.clientRuleWizardDescriptionPanel.Controls.Add(this.ruleDescriptionPanel);
            this.clientRuleWizardDescriptionPanel.Controls.Add(this.buttonAdvancedMode);
            this.clientRuleWizardDescriptionPanel.Controls.Add(this.label3);
            this.clientRuleWizardDescriptionPanel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.clientRuleWizardDescriptionPanel.Location = new System.Drawing.Point(282, 4);
            this.clientRuleWizardDescriptionPanel.Name = "clientRuleWizardDescriptionPanel";
            this.clientRuleWizardDescriptionPanel.Size = new System.Drawing.Size(400, 218);
            this.clientRuleWizardDescriptionPanel.TabIndex = 10;
            // 
            // panelComments
            // 
            this.panelComments.Controls.Add(this.groupBox1);
            this.panelComments.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.panelComments.Location = new System.Drawing.Point(4, 381);
            this.panelComments.Name = "panelComments";
            this.panelComments.Size = new System.Drawing.Size(677, 150);
            this.panelComments.TabIndex = 11;
            // 
            // groupBox1
            // 
            this.groupBox1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.label7);
            this.groupBox1.Controls.Add(this.label6);
            this.groupBox1.Controls.Add(this.label5);
            this.groupBox1.Controls.Add(this.textAdditionalComments);
            this.groupBox1.Controls.Add(this.textName);
            this.groupBox1.Controls.Add(this.textPlatform);
            this.groupBox1.Controls.Add(this.textEmail);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Location = new System.Drawing.Point(0, 0);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(677, 150);
            this.groupBox1.TabIndex = 8;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Rule Details";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.BackColor = System.Drawing.Color.Transparent;
            this.label4.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(5, 15);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(88, 13);
            this.label4.TabIndex = 1;
            this.label4.Text = "Creator Name:";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.BackColor = System.Drawing.Color.Transparent;
            this.label7.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label7.Location = new System.Drawing.Point(275, 16);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(131, 13);
            this.label7.TabIndex = 4;
            this.label7.Text = "Additional Comments:";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.BackColor = System.Drawing.Color.Transparent;
            this.label6.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(5, 102);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(177, 13);
            this.label6.TabIndex = 3;
            this.label6.Text = "Compatible Platform Versions:";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.BackColor = System.Drawing.Color.Transparent;
            this.label5.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(4, 56);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(86, 13);
            this.label5.TabIndex = 2;
            this.label5.Text = "Creator Email:";
            // 
            // textAdditionalComments
            // 
            this.textAdditionalComments.Location = new System.Drawing.Point(275, 33);
            this.textAdditionalComments.Name = "textAdditionalComments";
            this.textAdditionalComments.Size = new System.Drawing.Size(392, 105);
            this.textAdditionalComments.TabIndex = 7;
            this.textAdditionalComments.Text = "";
            // 
            // textName
            // 
            this.textName.Location = new System.Drawing.Point(8, 31);
            this.textName.Name = "textName";
            this.textName.Size = new System.Drawing.Size(245, 20);
            this.textName.TabIndex = 4;
            // 
            // textPlatform
            // 
            this.textPlatform.Location = new System.Drawing.Point(8, 118);
            this.textPlatform.Name = "textPlatform";
            this.textPlatform.Size = new System.Drawing.Size(177, 20);
            this.textPlatform.TabIndex = 6;
            // 
            // textEmail
            // 
            this.textEmail.Location = new System.Drawing.Point(7, 72);
            this.textEmail.Name = "textEmail";
            this.textEmail.Size = new System.Drawing.Size(245, 20);
            this.textEmail.TabIndex = 5;
            // 
            // clientQueryRuleBindingSource
            // 
            this.clientQueryRuleBindingSource.DataSource = typeof(Trafodion.Manager.WorkloadArea.Model.ClientRule);
            // 
            // helpButton
            // 
            this.helpButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.helpButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.helpButton.Location = new System.Drawing.Point(310, 109);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(80, 25);
            this.helpButton.TabIndex = 2;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // ClientRuleWizard
            // 
            this.AcceptButton = this.wizardFinish;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CancelButton = this.wizardCancel;
            this.ClientSize = new System.Drawing.Size(692, 541);
            this.Controls.Add(this.clientRuleWizardDescriptionPanel);
            this.Controls.Add(this.clientRuleWizardSelectionPanel);
            this.Controls.Add(this.panelComments);
            this.Controls.Add(this.clientRulesButtonPanel);
            this.clientRuleWizardHelpProvider.SetHelpKeyword(this, "ch05s10.html");
            this.clientRuleWizardHelpProvider.SetHelpNavigator(this, System.Windows.Forms.HelpNavigator.Topic);
            this.MaximizeBox = false;
            this.MinimumSize = new System.Drawing.Size(260, 38);
            this.Name = "ClientRuleWizard";
            this.clientRuleWizardHelpProvider.SetShowHelp(this, true);
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Threshold Rule Creation Wizard";
            this.clientRulesButtonPanel.ResumeLayout(false);
            this.clientRulesButtonPanel.PerformLayout();
            this.ruleDescriptionPanel.ResumeLayout(false);
            this.clientRuleWizardSelectionPanel.ResumeLayout(false);
            this.clientRuleWizardTabControl.ResumeLayout(false);
            this.tabPage_Systems.ResumeLayout(false);
            this.panel5.ResumeLayout(false);
            this.panel5.PerformLayout();
            this.clientRuleConditionsTabPage.ResumeLayout(false);
            this.clientRuleActionsTabPage.ResumeLayout(false);
            this.clientRuleWizardDescriptionPanel.ResumeLayout(false);
            this.clientRuleWizardDescriptionPanel.PerformLayout();
            this.panelComments.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.clientQueryRuleBindingSource)).EndInit();
            this.ResumeLayout(false);

        }
        #endregion

        private System.Windows.Forms.Panel clientRulesButtonPanel;
        private System.Windows.Forms.BindingSource clientQueryRuleBindingSource;
        private System.Windows.Forms.Panel ruleText;
        private System.Windows.Forms.Button wizardFinish;
        private System.Windows.Forms.Button wizardCancel;
        private System.Windows.Forms.Panel ruleDescriptionPanel;
        private System.Windows.Forms.Label ruleNameLabel;
        private System.Windows.Forms.TextBox textBox_QueryName;
        private System.Windows.Forms.Label enableRuleLabel;
        private System.Windows.Forms.CheckBox checkBox_RuleEnabled;
        private System.Windows.Forms.Panel clientRuleWizardSelectionPanel;
        private System.Windows.Forms.TabControl clientRuleWizardTabControl;
        private System.Windows.Forms.TabPage clientRuleConditionsTabPage;
        private System.Windows.Forms.TabPage clientRuleActionsTabPage;
        private System.Windows.Forms.ListView listView2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.ColumnHeader columnHeader3;
        private System.Windows.Forms.TreeView ruleConditionsTreeView;
        private System.Windows.Forms.Button buttonAdvancedMode;
        private System.Windows.Forms.Panel clientRuleWizardDescriptionPanel;
        private System.Windows.Forms.RichTextBox textBoxExpression;
        private System.Windows.Forms.Button buttonComments;
        private System.Windows.Forms.Panel panelComments;
        private System.Windows.Forms.TextBox textName;
        private System.Windows.Forms.TextBox textEmail;
        private System.Windows.Forms.TextBox textPlatform;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.RichTextBox textAdditionalComments;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button btnSaveRule;
        private System.Windows.Forms.TabPage tabPage_Systems;
        private System.Windows.Forms.TreeView treeView_Systems;
        private System.Windows.Forms.CheckBox checkBox_allSystem;
        private System.Windows.Forms.CheckBox checkBox_allConnect;
        private System.Windows.Forms.Panel panel5;
        private System.Windows.Forms.HelpProvider clientRuleWizardHelpProvider;
        private System.Windows.Forms.Button helpButton;
    }
}