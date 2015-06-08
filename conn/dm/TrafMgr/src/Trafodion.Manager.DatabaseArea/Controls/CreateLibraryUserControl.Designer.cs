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
    partial class CreateLibraryUserControl
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
            this._theLibName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.lblSchema = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblCatalog = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.lblLibrary = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theCloseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCreateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theHelpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.lblJarFile = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theJarFileName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theFileBrowserButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._codeFileGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._errorText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._statusStrip = new System.Windows.Forms.StatusStrip();
            this._progressBar = new System.Windows.Forms.ToolStripProgressBar();
            this._statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bannerControl = new Trafodion.Manager.Framework.Controls.TrafodionBannerControl();
            this._nameGroupBox.SuspendLayout();
            this._codeFileGroupBox.SuspendLayout();
            this._statusStrip.SuspendLayout();
            this._buttonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _nameGroupBox
            // 
            this._nameGroupBox.Controls.Add(this._theCatalogName);
            this._nameGroupBox.Controls.Add(this._theSchemaName);
            this._nameGroupBox.Controls.Add(this._theLibName);
            this._nameGroupBox.Controls.Add(this.lblSchema);
            this._nameGroupBox.Controls.Add(this.lblCatalog);
            this._nameGroupBox.Controls.Add(this.lblLibrary);
            this._nameGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._nameGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._nameGroupBox.Location = new System.Drawing.Point(0, 64);
            this._nameGroupBox.Margin = new System.Windows.Forms.Padding(5);
            this._nameGroupBox.Name = "_nameGroupBox";
            this._nameGroupBox.Size = new System.Drawing.Size(785, 115);
            this._nameGroupBox.TabIndex = 0;
            this._nameGroupBox.TabStop = false;
            this._nameGroupBox.Text = "Library";
            // 
            // _theCatalogName
            // 
            this._theCatalogName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theCatalogName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theCatalogName.Location = new System.Drawing.Point(90, 18);
            this._theCatalogName.Name = "_theCatalogName";
            this._theCatalogName.ReadOnly = true;
            this._theCatalogName.Size = new System.Drawing.Size(689, 20);
            this._theCatalogName.TabIndex = 1;
            this._theCatalogName.TabStop = false;
            // 
            // _theSchemaName
            // 
            this._theSchemaName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSchemaName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theSchemaName.Location = new System.Drawing.Point(90, 49);
            this._theSchemaName.Name = "_theSchemaName";
            this._theSchemaName.ReadOnly = true;
            this._theSchemaName.Size = new System.Drawing.Size(689, 20);
            this._theSchemaName.TabIndex = 3;
            this._theSchemaName.TabStop = false;
            // 
            // _theLibName
            // 
            this._theLibName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theLibName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theLibName.Location = new System.Drawing.Point(90, 80);
            this._theLibName.MaxLength = 128;
            this._theLibName.Name = "_theLibName";
            this._theLibName.Size = new System.Drawing.Size(689, 20);
            this._theLibName.TabIndex = 5;
            this._theLibName.TextChanged += new System.EventHandler(this._theLibName_TextChanged);
            // 
            // lblSchema
            // 
            this.lblSchema.AutoSize = true;
            this.lblSchema.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblSchema.Location = new System.Drawing.Point(35, 52);
            this.lblSchema.Name = "lblSchema";
            this.lblSchema.Size = new System.Drawing.Size(44, 13);
            this.lblSchema.TabIndex = 2;
            this.lblSchema.Text = "Schema";
            this.lblSchema.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // lblCatalog
            // 
            this.lblCatalog.AutoSize = true;
            this.lblCatalog.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblCatalog.Location = new System.Drawing.Point(35, 21);
            this.lblCatalog.Name = "lblCatalog";
            this.lblCatalog.Size = new System.Drawing.Size(44, 13);
            this.lblCatalog.TabIndex = 0;
            this.lblCatalog.Text = "Catalog";
            this.lblCatalog.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // lblLibrary
            // 
            this.lblLibrary.AutoSize = true;
            this.lblLibrary.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblLibrary.Location = new System.Drawing.Point(9, 83);
            this.lblLibrary.Name = "lblLibrary";
            this.lblLibrary.Size = new System.Drawing.Size(70, 13);
            this.lblLibrary.TabIndex = 4;
            this.lblLibrary.Text = "Library Name";
            this.lblLibrary.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theCloseButton
            // 
            this._theCloseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCloseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCloseButton.Location = new System.Drawing.Point(625, 3);
            this._theCloseButton.Name = "_theCloseButton";
            this._theCloseButton.Size = new System.Drawing.Size(75, 23);
            this._theCloseButton.TabIndex = 1;
            this._theCloseButton.Text = "&Close";
            this._theCloseButton.UseVisualStyleBackColor = true;
            this._theCloseButton.Click += new System.EventHandler(this._theCloseButton_Click);
            // 
            // _theCreateButton
            // 
            this._theCreateButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCreateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCreateButton.Location = new System.Drawing.Point(544, 3);
            this._theCreateButton.Name = "_theCreateButton";
            this._theCreateButton.Size = new System.Drawing.Size(75, 23);
            this._theCreateButton.TabIndex = 0;
            this._theCreateButton.Text = "C&reate";
            this._theCreateButton.UseVisualStyleBackColor = true;
            this._theCreateButton.Click += new System.EventHandler(this._theCreateButton_Click);
            // 
            // _theHelpButton
            // 
            this._theHelpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theHelpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theHelpButton.Location = new System.Drawing.Point(706, 3);
            this._theHelpButton.Name = "_theHelpButton";
            this._theHelpButton.Size = new System.Drawing.Size(75, 23);
            this._theHelpButton.TabIndex = 2;
            this._theHelpButton.Text = "He&lp";
            this._theHelpButton.UseVisualStyleBackColor = true;
            this._theHelpButton.Click += new System.EventHandler(this._theHelpButton_Click);
            // 
            // lblJarFile
            // 
            this.lblJarFile.AutoSize = true;
            this.lblJarFile.Font = new System.Drawing.Font("Tahoma", 8F);
            this.lblJarFile.Location = new System.Drawing.Point(28, 29);
            this.lblJarFile.Name = "lblJarFile";
            this.lblJarFile.Size = new System.Drawing.Size(51, 13);
            this.lblJarFile.TabIndex = 0;
            this.lblJarFile.Text = "Code File";
            this.lblJarFile.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _theJarFileName
            // 
            this._theJarFileName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theJarFileName.Font = new System.Drawing.Font("Tahoma", 8F);
            this._theJarFileName.Location = new System.Drawing.Point(90, 26);
            this._theJarFileName.Name = "_theJarFileName";
            this._theJarFileName.Size = new System.Drawing.Size(623, 20);
            this._theJarFileName.TabIndex = 1;
            this._theJarFileName.TextChanged += new System.EventHandler(this._theJarFileName_TextChanged);
            // 
            // _theFileBrowserButton
            // 
            this._theFileBrowserButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theFileBrowserButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theFileBrowserButton.Location = new System.Drawing.Point(719, 24);
            this._theFileBrowserButton.Name = "_theFileBrowserButton";
            this._theFileBrowserButton.Size = new System.Drawing.Size(60, 22);
            this._theFileBrowserButton.TabIndex = 2;
            this._theFileBrowserButton.Text = "&Browse...";
            this._theFileBrowserButton.UseVisualStyleBackColor = true;
            this._theFileBrowserButton.Click += new System.EventHandler(this._theFileBrowserButton_Click);
            // 
            // _codeFileGroupBox
            // 
            this._codeFileGroupBox.Controls.Add(this._theFileBrowserButton);
            this._codeFileGroupBox.Controls.Add(this._theJarFileName);
            this._codeFileGroupBox.Controls.Add(this.lblJarFile);
            this._codeFileGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._codeFileGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._codeFileGroupBox.Location = new System.Drawing.Point(0, 179);
            this._codeFileGroupBox.Margin = new System.Windows.Forms.Padding(5);
            this._codeFileGroupBox.Name = "_codeFileGroupBox";
            this._codeFileGroupBox.Size = new System.Drawing.Size(785, 67);
            this._codeFileGroupBox.TabIndex = 1;
            this._codeFileGroupBox.TabStop = false;
            this._codeFileGroupBox.Text = "Code File";
            // 
            // _errorText
            // 
            this._errorText.AutoSize = true;
            this._errorText.Dock = System.Windows.Forms.DockStyle.Top;
            this._errorText.Font = new System.Drawing.Font("Tahoma", 8F);
            this._errorText.ForeColor = System.Drawing.Color.Red;
            this._errorText.Location = new System.Drawing.Point(0, 51);
            this._errorText.Name = "_errorText";
            this._errorText.Size = new System.Drawing.Size(0, 13);
            this._errorText.TabIndex = 8;
            this._errorText.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // _statusStrip
            // 
            this._statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._progressBar,
            this._statusLabel});
            this._statusStrip.Location = new System.Drawing.Point(0, 351);
            this._statusStrip.Name = "_statusStrip";
            this._statusStrip.Size = new System.Drawing.Size(785, 22);
            this._statusStrip.TabIndex = 9;
            this._statusStrip.Text = "TrafodionStatusStrip1";
            // 
            // _progressBar
            // 
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(150, 16);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // _statusLabel
            // 
            this._statusLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this._statusLabel.Name = "_statusLabel";
            this._statusLabel.Size = new System.Drawing.Size(0, 17);
            this._statusLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._theCloseButton);
            this._buttonsPanel.Controls.Add(this._theCreateButton);
            this._buttonsPanel.Controls.Add(this._theHelpButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 322);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(785, 29);
            this._buttonsPanel.TabIndex = 2;
            // 
            // _bannerControl
            // 
            this._bannerControl.ConnectionDefinition = null;
            this._bannerControl.Dock = System.Windows.Forms.DockStyle.Top;
            this._bannerControl.Location = new System.Drawing.Point(0, 0);
            this._bannerControl.Name = "_bannerControl";
            this._bannerControl.ShowDescription = true;
            this._bannerControl.Size = new System.Drawing.Size(785, 51);
            this._bannerControl.TabIndex = 11;
            // 
            // CreateLibraryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(785, 373);
            this.Controls.Add(this._codeFileGroupBox);
            this.Controls.Add(this._nameGroupBox);
            this.Controls.Add(this._errorText);
            this.Controls.Add(this._bannerControl);
            this.Controls.Add(this._buttonsPanel);
            this.Controls.Add(this._statusStrip);
            this.Name = "CreateLibraryUserControl";
            this.Text = "Trafodion Database Manager - Create Library";
            this._nameGroupBox.ResumeLayout(false);
            this._nameGroupBox.PerformLayout();
            this._codeFileGroupBox.ResumeLayout(false);
            this._codeFileGroupBox.PerformLayout();
            this._statusStrip.ResumeLayout(false);
            this._statusStrip.PerformLayout();
            this._buttonsPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _nameGroupBox;
        private Framework.Controls.TrafodionTextBox _theLibName;
        private Framework.Controls.TrafodionLabel lblLibrary;
        private Framework.Controls.TrafodionButton _theCloseButton;
        private Framework.Controls.TrafodionButton _theCreateButton;
        private Framework.Controls.TrafodionButton _theHelpButton;
        private Framework.Controls.TrafodionLabel lblSchema;
        private Framework.Controls.TrafodionLabel lblCatalog;
        private Framework.Controls.TrafodionLabel lblJarFile;
        private Framework.Controls.TrafodionTextBox _theJarFileName;
        private Framework.Controls.TrafodionButton _theFileBrowserButton;
        private Framework.Controls.TrafodionGroupBox _codeFileGroupBox;
        private Framework.Controls.TrafodionTextBox _theCatalogName;
        private Framework.Controls.TrafodionTextBox _theSchemaName;
        private Framework.Controls.TrafodionLabel _errorText;
        private System.Windows.Forms.StatusStrip _statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel _statusLabel;
        private System.Windows.Forms.ToolStripProgressBar _progressBar;
        private Framework.Controls.TrafodionPanel _buttonsPanel;
        private Framework.Controls.TrafodionBannerControl _bannerControl;
    }
}
