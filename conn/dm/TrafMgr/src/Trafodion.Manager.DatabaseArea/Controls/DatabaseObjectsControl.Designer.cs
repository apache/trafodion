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
namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class DatabaseObjectsControl
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
            this._dataBaseObjectsToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);            
            this.theTabControlPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.topPanel_richTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.theTopPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // theTabControlPanel
            // 
            this.theTabControlPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTabControlPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theTabControlPanel.Location = new System.Drawing.Point(0, 33);
            this.theTabControlPanel.Name = "theTabControlPanel";
            this.theTabControlPanel.Size = new System.Drawing.Size(1024, 741);
            this.theTabControlPanel.TabIndex = 4;
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.theTopPanel.Controls.Add(this.topPanel_richTextBox);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(1024, 33);
            this.theTopPanel.TabIndex = 3;
            // 
            // topPanel_richTextBox
            // 
            this.topPanel_richTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.topPanel_richTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this.topPanel_richTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.topPanel_richTextBox.DetectUrls = false;
            this.topPanel_richTextBox.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.topPanel_richTextBox.Location = new System.Drawing.Point(5, 7);
            this.topPanel_richTextBox.Multiline = false;
            this.topPanel_richTextBox.Name = "topPanel_richTextBox";
            this.topPanel_richTextBox.ReadOnly = true;
            this.topPanel_richTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.None;
            this.topPanel_richTextBox.Size = new System.Drawing.Size(1001, 25);
            this.topPanel_richTextBox.TabIndex = 4;
            this.topPanel_richTextBox.TabStop = false;
            this.topPanel_richTextBox.Text = "";
            this.topPanel_richTextBox.WordWrap = false;
            // 
            // DatabaseObjectsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.theTabControlPanel);
            this.Controls.Add(this.theTopPanel);
            this.Name = "DatabaseObjectsControl";
            this.Size = new System.Drawing.Size(1024, 774);
            this.theTopPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionPanel theTopPanel;
        private TrafodionPanel theTabControlPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _dataBaseObjectsToolTip;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox topPanel_richTextBox;

    }
}
