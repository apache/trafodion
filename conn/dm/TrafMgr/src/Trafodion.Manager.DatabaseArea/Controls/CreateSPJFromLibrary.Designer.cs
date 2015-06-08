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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class CreateSPJFromLibrary
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
            this._nameGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theCatalogName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theSchemaName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.lblSchema = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblCata = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSPJName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._codeFileGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._methodNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._classNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theFileBrowserButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLibFileName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._parametersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.gridPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._editButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._attributesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theTransactionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theTransNoRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.TrafodionLabel7 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theTransYesRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theExternalSecurityGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theDefinerRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.TrafodionLabel5 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theInvokerRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theAccessesDatabase = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._theDynamicResultSets = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.TrafodionLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theCreateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._errorText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._bannerControl = new Trafodion.Manager.Framework.Controls.TrafodionBannerControl();
            this._nameGroupBox.SuspendLayout();
            this._codeFileGroupBox.SuspendLayout();
            this._parametersGroupBox.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this._attributesGroupBox.SuspendLayout();
            this._theTransactionGroupBox.SuspendLayout();
            this._theExternalSecurityGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theDynamicResultSets)).BeginInit();
            this._buttonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _nameGroupBox
            // 
            this._nameGroupBox.Controls.Add(this._theCatalogName);
            this._nameGroupBox.Controls.Add(this._theSchemaName);
            this._nameGroupBox.Controls.Add(this.lblSchema);
            this._nameGroupBox.Controls.Add(this.lblCata);
            this._nameGroupBox.Controls.Add(this._theSPJName);
            this._nameGroupBox.Controls.Add(this.TrafodionLabel1);
            this._nameGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._nameGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._nameGroupBox.Location = new System.Drawing.Point(0, 65);
            this._nameGroupBox.Name = "_nameGroupBox";
            this._nameGroupBox.Size = new System.Drawing.Size(905, 104);
            this._nameGroupBox.TabIndex = 0;
            this._nameGroupBox.TabStop = false;
            this._nameGroupBox.Text = "Name";
            // 
            // _theCatalogName
            // 
            this._theCatalogName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theCatalogName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theCatalogName.Location = new System.Drawing.Point(102, 17);
            this._theCatalogName.Name = "_theCatalogName";
            this._theCatalogName.ReadOnly = true;
            this._theCatalogName.Size = new System.Drawing.Size(779, 20);
            this._theCatalogName.TabIndex = 8;
            this._theCatalogName.TabStop = false;
            // 
            // _theSchemaName
            // 
            this._theSchemaName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSchemaName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theSchemaName.Location = new System.Drawing.Point(102, 44);
            this._theSchemaName.Name = "_theSchemaName";
            this._theSchemaName.ReadOnly = true;
            this._theSchemaName.Size = new System.Drawing.Size(779, 20);
            this._theSchemaName.TabIndex = 7;
            this._theSchemaName.TabStop = false;
            // 
            // lblSchema
            // 
            this.lblSchema.AutoSize = true;
            this.lblSchema.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblSchema.Location = new System.Drawing.Point(51, 47);
            this.lblSchema.Name = "lblSchema";
            this.lblSchema.Size = new System.Drawing.Size(44, 13);
            this.lblSchema.TabIndex = 6;
            this.lblSchema.Text = "Schema";
            this.lblSchema.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // lblCata
            // 
            this.lblCata.AutoSize = true;
            this.lblCata.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblCata.Location = new System.Drawing.Point(51, 20);
            this.lblCata.Name = "lblCata";
            this.lblCata.Size = new System.Drawing.Size(44, 13);
            this.lblCata.TabIndex = 5;
            this.lblCata.Text = "Catalog";
            this.lblCata.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theSPJName
            // 
            this._theSPJName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSPJName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theSPJName.Location = new System.Drawing.Point(102, 71);
            this._theSPJName.MaxLength = 128;
            this._theSPJName.Name = "_theSPJName";
            this._theSPJName.Size = new System.Drawing.Size(779, 20);
            this._theSPJName.TabIndex = 1;
            this._theSPJName.TextChanged += new System.EventHandler(this._theSPJName_TextChanged);
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(9, 74);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(86, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Procedure Name";
            this.TrafodionLabel1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _codeFileGroupBox
            // 
            this._codeFileGroupBox.Controls.Add(this._methodNameTextBox);
            this._codeFileGroupBox.Controls.Add(this._classNameTextBox);
            this._codeFileGroupBox.Controls.Add(this._theFileBrowserButton);
            this._codeFileGroupBox.Controls.Add(this.TrafodionLabel8);
            this._codeFileGroupBox.Controls.Add(this.TrafodionLabel4);
            this._codeFileGroupBox.Controls.Add(this.TrafodionLabel2);
            this._codeFileGroupBox.Controls.Add(this._theLibFileName);
            this._codeFileGroupBox.Controls.Add(this.TrafodionLabel3);
            this._codeFileGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._codeFileGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._codeFileGroupBox.Location = new System.Drawing.Point(0, 169);
            this._codeFileGroupBox.Name = "_codeFileGroupBox";
            this._codeFileGroupBox.Size = new System.Drawing.Size(905, 126);
            this._codeFileGroupBox.TabIndex = 1;
            this._codeFileGroupBox.TabStop = false;
            this._codeFileGroupBox.Text = "Code";
            // 
            // _methodNameTextBox
            // 
            this._methodNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._methodNameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._methodNameTextBox.Location = new System.Drawing.Point(101, 97);
            this._methodNameTextBox.Name = "_methodNameTextBox";
            this._methodNameTextBox.ReadOnly = true;
            this._methodNameTextBox.Size = new System.Drawing.Size(780, 20);
            this._methodNameTextBox.TabIndex = 10;
            // 
            // _classNameTextBox
            // 
            this._classNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._classNameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._classNameTextBox.Location = new System.Drawing.Point(101, 68);
            this._classNameTextBox.Name = "_classNameTextBox";
            this._classNameTextBox.ReadOnly = true;
            this._classNameTextBox.Size = new System.Drawing.Size(780, 20);
            this._classNameTextBox.TabIndex = 11;
            // 
            // _theFileBrowserButton
            // 
            this._theFileBrowserButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theFileBrowserButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theFileBrowserButton.Location = new System.Drawing.Point(821, 11);
            this._theFileBrowserButton.Name = "_theFileBrowserButton";
            this._theFileBrowserButton.Size = new System.Drawing.Size(60, 22);
            this._theFileBrowserButton.TabIndex = 7;
            this._theFileBrowserButton.Text = "&Browse...";
            this._theFileBrowserButton.UseVisualStyleBackColor = true;
            this._theFileBrowserButton.Click += new System.EventHandler(this._theFileBrowserButton_Click);
            // 
            // TrafodionLabel8
            // 
            this.TrafodionLabel8.AutoSize = true;
            this.TrafodionLabel8.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel8.Location = new System.Drawing.Point(22, 100);
            this.TrafodionLabel8.Name = "TrafodionLabel8";
            this.TrafodionLabel8.Size = new System.Drawing.Size(73, 13);
            this.TrafodionLabel8.TabIndex = 9;
            this.TrafodionLabel8.Text = "Method Name";
            this.TrafodionLabel8.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel4.Location = new System.Drawing.Point(33, 71);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(62, 13);
            this.TrafodionLabel4.TabIndex = 8;
            this.TrafodionLabel4.Text = "Class Name";
            this.TrafodionLabel4.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TrafodionLabel2.ForeColor = System.Drawing.Color.Blue;
            this.TrafodionLabel2.Location = new System.Drawing.Point(54, 15);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(314, 14);
            this.TrafodionLabel2.TabIndex = 0;
            this.TrafodionLabel2.Text = "Click Browse to select a library class method for this procedure";
            this.TrafodionLabel2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theLibFileName
            // 
            this._theLibFileName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theLibFileName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theLibFileName.Location = new System.Drawing.Point(101, 39);
            this._theLibFileName.Name = "_theLibFileName";
            this._theLibFileName.ReadOnly = true;
            this._theLibFileName.Size = new System.Drawing.Size(780, 20);
            this._theLibFileName.TabIndex = 2;
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(55, 42);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(40, 13);
            this.TrafodionLabel3.TabIndex = 1;
            this.TrafodionLabel3.Text = "Library";
            this.TrafodionLabel3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _parametersGroupBox
            // 
            this._parametersGroupBox.Controls.Add(this.gridPanel);
            this._parametersGroupBox.Controls.Add(this.TrafodionPanel1);
            this._parametersGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._parametersGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._parametersGroupBox.Location = new System.Drawing.Point(0, 295);
            this._parametersGroupBox.Name = "_parametersGroupBox";
            this._parametersGroupBox.Size = new System.Drawing.Size(905, 191);
            this._parametersGroupBox.TabIndex = 2;
            this._parametersGroupBox.TabStop = false;
            this._parametersGroupBox.Text = "Parameters";
            // 
            // gridPanel
            // 
            this.gridPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.gridPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.gridPanel.Location = new System.Drawing.Point(3, 17);
            this.gridPanel.Name = "gridPanel";
            this.gridPanel.Size = new System.Drawing.Size(899, 137);
            this.gridPanel.TabIndex = 2;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._editButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(3, 154);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(899, 34);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // _editButton
            // 
            this._editButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._editButton.Enabled = false;
            this._editButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._editButton.Location = new System.Drawing.Point(823, 6);
            this._editButton.Name = "_editButton";
            this._editButton.Size = new System.Drawing.Size(75, 23);
            this._editButton.TabIndex = 0;
            this._editButton.Text = "Edi&t...";
            this._editButton.UseVisualStyleBackColor = true;
            this._editButton.Click += new System.EventHandler(this._editButton_Click);
            // 
            // _attributesGroupBox
            // 
            this._attributesGroupBox.Controls.Add(this._theTransactionGroupBox);
            this._attributesGroupBox.Controls.Add(this._theExternalSecurityGroupBox);
            this._attributesGroupBox.Controls.Add(this._theAccessesDatabase);
            this._attributesGroupBox.Controls.Add(this._theDynamicResultSets);
            this._attributesGroupBox.Controls.Add(this.TrafodionLabel6);
            this._attributesGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._attributesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._attributesGroupBox.Location = new System.Drawing.Point(0, 486);
            this._attributesGroupBox.Name = "_attributesGroupBox";
            this._attributesGroupBox.Size = new System.Drawing.Size(905, 57);
            this._attributesGroupBox.TabIndex = 3;
            this._attributesGroupBox.TabStop = false;
            this._attributesGroupBox.Text = "Attributes";
            // 
            // _theTransactionGroupBox
            // 
            this._theTransactionGroupBox.Controls.Add(this._theTransNoRadioButton);
            this._theTransactionGroupBox.Controls.Add(this.TrafodionLabel7);
            this._theTransactionGroupBox.Controls.Add(this._theTransYesRadioButton);
            this._theTransactionGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTransactionGroupBox.Location = new System.Drawing.Point(645, 10);
            this._theTransactionGroupBox.Name = "_theTransactionGroupBox";
            this._theTransactionGroupBox.Size = new System.Drawing.Size(248, 38);
            this._theTransactionGroupBox.TabIndex = 7;
            this._theTransactionGroupBox.TabStop = false;
            // 
            // _theTransNoRadioButton
            // 
            this._theTransNoRadioButton.AutoSize = true;
            this._theTransNoRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theTransNoRadioButton.Location = new System.Drawing.Point(190, 16);
            this._theTransNoRadioButton.Margin = new System.Windows.Forms.Padding(0);
            this._theTransNoRadioButton.Name = "_theTransNoRadioButton";
            this._theTransNoRadioButton.Size = new System.Drawing.Size(44, 18);
            this._theTransNoRadioButton.TabIndex = 4;
            this._theTransNoRadioButton.TabStop = true;
            this._theTransNoRadioButton.Text = "No";
            this._theTransNoRadioButton.UseVisualStyleBackColor = true;
            // 
            // TrafodionLabel7
            // 
            this.TrafodionLabel7.AutoSize = true;
            this.TrafodionLabel7.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel7.Location = new System.Drawing.Point(13, 18);
            this.TrafodionLabel7.Margin = new System.Windows.Forms.Padding(0);
            this.TrafodionLabel7.Name = "TrafodionLabel7";
            this.TrafodionLabel7.Size = new System.Drawing.Size(109, 13);
            this.TrafodionLabel7.TabIndex = 3;
            this.TrafodionLabel7.Text = "Transaction Required";
            this.TrafodionLabel7.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theTransYesRadioButton
            // 
            this._theTransYesRadioButton.AutoSize = true;
            this._theTransYesRadioButton.Checked = true;
            this._theTransYesRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theTransYesRadioButton.Location = new System.Drawing.Point(141, 16);
            this._theTransYesRadioButton.Margin = new System.Windows.Forms.Padding(0);
            this._theTransYesRadioButton.Name = "_theTransYesRadioButton";
            this._theTransYesRadioButton.Size = new System.Drawing.Size(48, 18);
            this._theTransYesRadioButton.TabIndex = 3;
            this._theTransYesRadioButton.TabStop = true;
            this._theTransYesRadioButton.Text = "Yes";
            this._theTransYesRadioButton.UseVisualStyleBackColor = true;
            // 
            // _theExternalSecurityGroupBox
            // 
            this._theExternalSecurityGroupBox.Controls.Add(this._theDefinerRadioButton);
            this._theExternalSecurityGroupBox.Controls.Add(this.TrafodionLabel5);
            this._theExternalSecurityGroupBox.Controls.Add(this._theInvokerRadioButton);
            this._theExternalSecurityGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theExternalSecurityGroupBox.Location = new System.Drawing.Point(375, 10);
            this._theExternalSecurityGroupBox.Name = "_theExternalSecurityGroupBox";
            this._theExternalSecurityGroupBox.Size = new System.Drawing.Size(264, 38);
            this._theExternalSecurityGroupBox.TabIndex = 6;
            this._theExternalSecurityGroupBox.TabStop = false;
            // 
            // _theDefinerRadioButton
            // 
            this._theDefinerRadioButton.AutoSize = true;
            this._theDefinerRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theDefinerRadioButton.Location = new System.Drawing.Point(182, 16);
            this._theDefinerRadioButton.Margin = new System.Windows.Forms.Padding(0);
            this._theDefinerRadioButton.Name = "_theDefinerRadioButton";
            this._theDefinerRadioButton.Size = new System.Drawing.Size(66, 18);
            this._theDefinerRadioButton.TabIndex = 4;
            this._theDefinerRadioButton.TabStop = true;
            this._theDefinerRadioButton.Text = "Definer";
            this._theDefinerRadioButton.UseVisualStyleBackColor = true;
            // 
            // TrafodionLabel5
            // 
            this.TrafodionLabel5.AutoSize = true;
            this.TrafodionLabel5.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel5.Location = new System.Drawing.Point(13, 18);
            this.TrafodionLabel5.Margin = new System.Windows.Forms.Padding(0);
            this.TrafodionLabel5.Name = "TrafodionLabel5";
            this.TrafodionLabel5.Size = new System.Drawing.Size(89, 13);
            this.TrafodionLabel5.TabIndex = 3;
            this.TrafodionLabel5.Text = "External Security\r\n";
            this.TrafodionLabel5.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theInvokerRadioButton
            // 
            this._theInvokerRadioButton.AutoSize = true;
            this._theInvokerRadioButton.Checked = true;
            this._theInvokerRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theInvokerRadioButton.Location = new System.Drawing.Point(108, 16);
            this._theInvokerRadioButton.Margin = new System.Windows.Forms.Padding(0);
            this._theInvokerRadioButton.Name = "_theInvokerRadioButton";
            this._theInvokerRadioButton.Size = new System.Drawing.Size(68, 18);
            this._theInvokerRadioButton.TabIndex = 3;
            this._theInvokerRadioButton.TabStop = true;
            this._theInvokerRadioButton.Text = "Invoker";
            this._theInvokerRadioButton.UseVisualStyleBackColor = true;
            // 
            // _theAccessesDatabase
            // 
            this._theAccessesDatabase.AutoSize = true;
            this._theAccessesDatabase.Checked = true;
            this._theAccessesDatabase.CheckState = System.Windows.Forms.CheckState.Checked;
            this._theAccessesDatabase.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAccessesDatabase.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theAccessesDatabase.Location = new System.Drawing.Point(244, 26);
            this._theAccessesDatabase.Name = "_theAccessesDatabase";
            this._theAccessesDatabase.Size = new System.Drawing.Size(125, 18);
            this._theAccessesDatabase.TabIndex = 2;
            this._theAccessesDatabase.Text = "Accesses Database";
            this._theAccessesDatabase.UseVisualStyleBackColor = true;
            this._theAccessesDatabase.Click += new System.EventHandler(this._theAccessesDatabase_Click);
            // 
            // _theDynamicResultSets
            // 
            this._theDynamicResultSets.Enabled = false;
            this._theDynamicResultSets.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theDynamicResultSets.Location = new System.Drawing.Point(169, 24);
            this._theDynamicResultSets.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this._theDynamicResultSets.Name = "_theDynamicResultSets";
            this._theDynamicResultSets.Size = new System.Drawing.Size(46, 20);
            this._theDynamicResultSets.TabIndex = 1;
            this._theDynamicResultSets.ValueChanged += new System.EventHandler(this._theDynamicResultSets_ValueChanged);
            // 
            // TrafodionLabel6
            // 
            this.TrafodionLabel6.AutoSize = true;
            this.TrafodionLabel6.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel6.Location = new System.Drawing.Point(9, 26);
            this.TrafodionLabel6.Name = "TrafodionLabel6";
            this.TrafodionLabel6.Size = new System.Drawing.Size(152, 13);
            this.TrafodionLabel6.TabIndex = 0;
            this.TrafodionLabel6.Text = "Number of dynamic result sets";
            this.TrafodionLabel6.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theCreateButton
            // 
            this._theCreateButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCreateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCreateButton.Location = new System.Drawing.Point(661, 6);
            this._theCreateButton.Name = "_theCreateButton";
            this._theCreateButton.Size = new System.Drawing.Size(75, 23);
            this._theCreateButton.TabIndex = 4;
            this._theCreateButton.Text = "C&reate";
            this._theCreateButton.UseVisualStyleBackColor = true;
            this._theCreateButton.Click += new System.EventHandler(this._theCreateButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(742, 6);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 5;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _errorText
            // 
            this._errorText.AutoSize = true;
            this._errorText.Dock = System.Windows.Forms.DockStyle.Top;
            this._errorText.Font = new System.Drawing.Font("Tahoma", 8F);
            this._errorText.ForeColor = System.Drawing.Color.Red;
            this._errorText.Location = new System.Drawing.Point(0, 52);
            this._errorText.Name = "_errorText";
            this._errorText.Size = new System.Drawing.Size(0, 13);
            this._errorText.TabIndex = 0;
            this._errorText.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._theCreateButton);
            this._buttonsPanel.Controls.Add(this._helpButton);
            this._buttonsPanel.Controls.Add(this._theCancelButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 550);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(905, 32);
            this._buttonsPanel.TabIndex = 6;
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.Location = new System.Drawing.Point(823, 6);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 5;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // _bannerControl
            // 
            this._bannerControl.ConnectionDefinition = null;
            this._bannerControl.Dock = System.Windows.Forms.DockStyle.Top;
            this._bannerControl.Location = new System.Drawing.Point(0, 0);
            this._bannerControl.Name = "_bannerControl";
            this._bannerControl.ShowDescription = true;
            this._bannerControl.Size = new System.Drawing.Size(905, 52);
            this._bannerControl.TabIndex = 7;
            // 
            // CreateSPJFromLibrary
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(905, 582);
            this.Controls.Add(this._attributesGroupBox);
            this.Controls.Add(this._parametersGroupBox);
            this.Controls.Add(this._codeFileGroupBox);
            this.Controls.Add(this._nameGroupBox);
            this.Controls.Add(this._errorText);
            this.Controls.Add(this._bannerControl);
            this.Controls.Add(this._buttonsPanel);
            this.Name = "CreateSPJFromLibrary";
            this.Text = "Trafodion Database Manager - Create Procedure";
            this._nameGroupBox.ResumeLayout(false);
            this._nameGroupBox.PerformLayout();
            this._codeFileGroupBox.ResumeLayout(false);
            this._codeFileGroupBox.PerformLayout();
            this._parametersGroupBox.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this._attributesGroupBox.ResumeLayout(false);
            this._attributesGroupBox.PerformLayout();
            this._theTransactionGroupBox.ResumeLayout(false);
            this._theTransactionGroupBox.PerformLayout();
            this._theExternalSecurityGroupBox.ResumeLayout(false);
            this._theExternalSecurityGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theDynamicResultSets)).EndInit();
            this._buttonsPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _nameGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theSPJName;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _codeFileGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theLibFileName;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theFileBrowserButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _parametersGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _attributesGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCreateButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _theAccessesDatabase;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _theDynamicResultSets;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel6;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _editButton;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _methodNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _classNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel8;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel4;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel gridPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _errorText;
        private Framework.Controls.TrafodionGroupBox _theExternalSecurityGroupBox;
        private Framework.Controls.TrafodionRadioButton _theDefinerRadioButton;
        private Framework.Controls.TrafodionLabel TrafodionLabel5;
        private Framework.Controls.TrafodionRadioButton _theInvokerRadioButton;
        private Framework.Controls.TrafodionLabel lblSchema;
        private Framework.Controls.TrafodionLabel lblCata;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionPanel _buttonsPanel;
        private Framework.Controls.TrafodionButton _helpButton;
        private Framework.Controls.TrafodionTextBox _theCatalogName;
        private Framework.Controls.TrafodionTextBox _theSchemaName;
        private Framework.Controls.TrafodionBannerControl _bannerControl;
        private Framework.Controls.TrafodionGroupBox _theTransactionGroupBox;
        private Framework.Controls.TrafodionRadioButton _theTransNoRadioButton;
        private Framework.Controls.TrafodionLabel TrafodionLabel7;
        private Framework.Controls.TrafodionRadioButton _theTransYesRadioButton;
    }
}
