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
﻿using System.Drawing;
namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionMultipleMessageDialog
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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle4 = new System.Windows.Forms.DataGridViewCellStyle();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionMultipleMessageDialog));
            this.messagesGridView = new Trafodion.Manager.Framework.Controls.TrafodionDataGridView();
            this.pictureBox1 = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.summaryMessageLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.showDetailsButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.OkButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.gbxDetail = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            ((System.ComponentModel.ISupportInitialize)(this.messagesGridView)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.gbxDetail.SuspendLayout();
            this.SuspendLayout();
            // 
            // messagesGridView
            // 
            this.messagesGridView.AllowUserToAddRows = false;
            this.messagesGridView.AllowUserToDeleteRows = false;
            this.messagesGridView.AllowUserToOrderColumns = true;
            dataGridViewCellStyle1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.messagesGridView.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle1;
            this.messagesGridView.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.messagesGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCells;
            this.messagesGridView.BackgroundColor = System.Drawing.Color.WhiteSmoke;
            this.messagesGridView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.messagesGridView.ClipboardCopyMode = System.Windows.Forms.DataGridViewClipboardCopyMode.EnableAlwaysIncludeHeaderText;
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.messagesGridView.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle2;
            this.messagesGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle3.BackColor = System.Drawing.Color.WhiteSmoke;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle3.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.messagesGridView.DefaultCellStyle = dataGridViewCellStyle3;
            this.messagesGridView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.messagesGridView.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this.messagesGridView.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.messagesGridView.GridColor = System.Drawing.Color.LightGray;
            this.messagesGridView.Location = new System.Drawing.Point(6, 14);
            this.messagesGridView.Margin = new System.Windows.Forms.Padding(0);
            this.messagesGridView.Name = "messagesGridView";
            this.messagesGridView.ReadOnly = true;
            dataGridViewCellStyle4.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle4.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle4.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle4.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle4.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle4.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle4.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.messagesGridView.RowHeadersDefaultCellStyle = dataGridViewCellStyle4;
            this.messagesGridView.RowHeadersVisible = false;
            this.messagesGridView.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.AutoSizeToAllHeaders;
            this.messagesGridView.RowTemplate.ReadOnly = true;
            this.messagesGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.messagesGridView.Size = new System.Drawing.Size(686, 102);
            this.messagesGridView.TabIndex = 9;
            this.messagesGridView.TheHeaderText = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            // 
            // pictureBox1
            // 
            this.pictureBox1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(12, 20);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(35, 34);
            this.pictureBox1.TabIndex = 10;
            this.pictureBox1.TabStop = false;
            // 
            // summaryMessageLabel
            // 
            this.summaryMessageLabel.AutoSize = true;
            this.summaryMessageLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.summaryMessageLabel.Location = new System.Drawing.Point(53, 9);
            this.summaryMessageLabel.Name = "summaryMessageLabel";
            this.summaryMessageLabel.Padding = new System.Windows.Forms.Padding(0, 20, 0, 0);
            this.summaryMessageLabel.Size = new System.Drawing.Size(272, 33);
            this.summaryMessageLabel.TabIndex = 6;
            this.summaryMessageLabel.Text = "There were plugins that were invalid and were ignored.";
            // 
            // showDetailsButton
            // 
            this.showDetailsButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.showDetailsButton.Location = new System.Drawing.Point(230, 57);
            this.showDetailsButton.Name = "showDetailsButton";
            this.showDetailsButton.Size = new System.Drawing.Size(106, 23);
            this.showDetailsButton.TabIndex = 8;
            this.showDetailsButton.Text = "Show Details";
            this.showDetailsButton.UseVisualStyleBackColor = true;
            this.showDetailsButton.Click += new System.EventHandler(this.showDetails_Click);
            // 
            // OkButton
            // 
            this.OkButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.OkButton.Location = new System.Drawing.Point(363, 57);
            this.OkButton.Name = "OkButton";
            this.OkButton.Size = new System.Drawing.Size(76, 23);
            this.OkButton.TabIndex = 7;
            this.OkButton.Text = "OK";
            this.OkButton.UseVisualStyleBackColor = true;
            this.OkButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // gbxDetail
            // 
            this.gbxDetail.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.gbxDetail.Controls.Add(this.messagesGridView);
            this.gbxDetail.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.gbxDetail.Location = new System.Drawing.Point(11, 89);
            this.gbxDetail.Margin = new System.Windows.Forms.Padding(0);
            this.gbxDetail.Name = "gbxDetail";
            this.gbxDetail.Padding = new System.Windows.Forms.Padding(6, 0, 6, 8);
            this.gbxDetail.Size = new System.Drawing.Size(698, 124);
            this.gbxDetail.TabIndex = 11;
            this.gbxDetail.TabStop = false;
            // 
            // TrafodionMultipleMessageDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.ClientSize = new System.Drawing.Size(721, 223);
            this.Controls.Add(this.gbxDetail);
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.summaryMessageLabel);
            this.Controls.Add(this.showDetailsButton);
            this.Controls.Add(this.OkButton);
            this.MinimumSize = new System.Drawing.Size(600, 140);
            this.Name = "TrafodionMultipleMessageDialog";
            this.Text = "Trafodion Database Manager - Message(s)";
            ((System.ComponentModel.ISupportInitialize)(this.messagesGridView)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.gbxDetail.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private TrafodionDataGridView messagesGridView;
        private TrafodionPictureBox pictureBox1;
        private TrafodionLabel summaryMessageLabel;
        private TrafodionButton showDetailsButton;
        private TrafodionButton OkButton;
        private TrafodionGroupBox gbxDetail;


    }
}
