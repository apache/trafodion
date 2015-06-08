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
    partial class TrafodionConfirmationDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionConfirmationDialog));
            this.buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._yesToAllButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._noButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._yesButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.paramPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.pictureBox1 = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.prompt_TrafodionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.buttonsPanel.SuspendLayout();
            this.paramPanel.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // buttonsPanel
            // 
            this.buttonsPanel.Controls.Add(this._cancelButton);
            this.buttonsPanel.Controls.Add(this._yesToAllButton);
            this.buttonsPanel.Controls.Add(this._noButton);
            this.buttonsPanel.Controls.Add(this._yesButton);
            this.buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.buttonsPanel.Location = new System.Drawing.Point(0, 134);
            this.buttonsPanel.Name = "buttonsPanel";
            this.buttonsPanel.Size = new System.Drawing.Size(341, 40);
            this.buttonsPanel.TabIndex = 0;
            // 
            // _cancelButton
            // 
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Location = new System.Drawing.Point(255, 7);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 25);
            this._cancelButton.TabIndex = 2;
            this._cancelButton.Text = "Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            // 
            // _yesToAllButton
            // 
            this._yesToAllButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._yesToAllButton.Location = new System.Drawing.Point(93, 7);
            this._yesToAllButton.Name = "_yesToAllButton";
            this._yesToAllButton.Size = new System.Drawing.Size(75, 25);
            this._yesToAllButton.TabIndex = 1;
            this._yesToAllButton.Text = "Yes to All";
            this._yesToAllButton.UseVisualStyleBackColor = true;
            this._yesToAllButton.Click += new System.EventHandler(this._yesToAllButton_Click);
            // 
            // _noButton
            // 
            this._noButton.DialogResult = System.Windows.Forms.DialogResult.No;
            this._noButton.Location = new System.Drawing.Point(174, 7);
            this._noButton.Name = "_noButton";
            this._noButton.Size = new System.Drawing.Size(75, 25);
            this._noButton.TabIndex = 0;
            this._noButton.Text = "No";
            this._noButton.UseVisualStyleBackColor = true;
            // 
            // _yesButton
            // 
            this._yesButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._yesButton.Location = new System.Drawing.Point(13, 7);
            this._yesButton.Name = "_yesButton";
            this._yesButton.Size = new System.Drawing.Size(75, 25);
            this._yesButton.TabIndex = 0;
            this._yesButton.Text = "Yes";
            this._yesButton.UseVisualStyleBackColor = true;
            this._yesButton.Click += new System.EventHandler(this._yesButton_Click);
            // 
            // paramPanel
            // 
            this.paramPanel.Controls.Add(this.tableLayoutPanel1);
            this.paramPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.paramPanel.Location = new System.Drawing.Point(0, 0);
            this.paramPanel.Name = "paramPanel";
            this.paramPanel.Size = new System.Drawing.Size(341, 174);
            this.paramPanel.TabIndex = 0;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tableLayoutPanel1.ColumnCount = 2;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 246F));
            this.tableLayoutPanel1.Controls.Add(this.pictureBox1, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.prompt_TrafodionLabel, 1, 0);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(341, 135);
            this.tableLayoutPanel1.TabIndex = 1;
            // 
            // pictureBox1
            // 
            this.pictureBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.pictureBox1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(31, 50);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(33, 34);
            this.pictureBox1.TabIndex = 6;
            this.pictureBox1.TabStop = false;
            // 
            // prompt_TrafodionLabel
            // 
            this.prompt_TrafodionLabel.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.prompt_TrafodionLabel.AutoSize = true;
            this.prompt_TrafodionLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.prompt_TrafodionLabel.Location = new System.Drawing.Point(98, 60);
            this.prompt_TrafodionLabel.Name = "prompt_TrafodionLabel";
            this.prompt_TrafodionLabel.Size = new System.Drawing.Size(77, 14);
            this.prompt_TrafodionLabel.TabIndex = 0;
            this.prompt_TrafodionLabel.Text = "Are you sure?";
            // 
            // TrafodionConfirmationDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.ClientSize = new System.Drawing.Size(341, 174);
            this.Controls.Add(this.buttonsPanel);
            this.Controls.Add(this.paramPanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "TrafodionConfirmationDialog";
            this.Text = "Trafodion Database Manager - Confirm Dialog";
            this.buttonsPanel.ResumeLayout(false);
            this.paramPanel.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel paramPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _yesButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _noButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _yesToAllButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel prompt_TrafodionLabel;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPictureBox pictureBox1;
    }
}