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
﻿using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryListUserControl
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
            this.components = new System.ComponentModel.Container();
            Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(QueryListUserControl));
            Trafodion.Manager.Framework.Controls.TrafodionPanel panel2;
            this._checkAllCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this._theLoadStatementResultsButton = new System.Windows.Forms.ToolStripButton();
            this._theSaveStatementResultsButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._discardStatementButton = new System.Windows.Forms.ToolStripButton();
            this._theHelpStripButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this._moveToPersistenceFileButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this._moveOneUpButton = new System.Windows.Forms.ToolStripButton();
            this._moveOneDownButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripLabel2 = new System.Windows.Forms.ToolStripLabel();
            this._theBatchExecuteButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theBatchExplainButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOpenFileDialog = new System.Windows.Forms.OpenFileDialog();
            this._theSaveFileDialog = new System.Windows.Forms.SaveFileDialog();
            this._theToolTips = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            panel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            panel1.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            panel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            panel1.Controls.Add(this._checkAllCheckBox);
            panel1.Controls.Add(this.toolStrip1);
            panel1.Dock = System.Windows.Forms.DockStyle.Top;
            panel1.Location = new System.Drawing.Point(0, 0);
            panel1.Name = "panel1";
            panel1.Size = new System.Drawing.Size(470, 30);
            panel1.TabIndex = 2;
            // 
            // _checkAllCheckBox
            // 
            this._checkAllCheckBox.AutoSize = true;
            this._checkAllCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._checkAllCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._checkAllCheckBox.Location = new System.Drawing.Point(24, 6);
            this._checkAllCheckBox.Name = "_checkAllCheckBox";
            this._checkAllCheckBox.Size = new System.Drawing.Size(75, 18);
            this._checkAllCheckBox.TabIndex = 5;
            this._checkAllCheckBox.Text = "Check All";
            this._checkAllCheckBox.UseVisualStyleBackColor = true;
            this._checkAllCheckBox.Click += new System.EventHandler(this.CheckAllCheckBox_Click);
            // 
            // toolStrip1
            // 
            this.toolStrip1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.toolStrip1.AutoSize = false;
            this.toolStrip1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.toolStrip1.Dock = System.Windows.Forms.DockStyle.None;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theLoadStatementResultsButton,
            this._theSaveStatementResultsButton,
            this.toolStripSeparator1,
            this._discardStatementButton,
            this._theHelpStripButton,
            this.toolStripSeparator2,
            this._moveToPersistenceFileButton,
            this.toolStripSeparator3,
            this._moveOneUpButton,
            this._moveOneDownButton,
            this.toolStripSeparator4,
            this.toolStripLabel2});
            this.toolStrip1.Location = new System.Drawing.Point(104, 3);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.toolStrip1.Size = new System.Drawing.Size(366, 25);
            this.toolStrip1.TabIndex = 4;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // _theLoadStatementResultsButton
            // 
            this._theLoadStatementResultsButton.BackColor = System.Drawing.Color.Snow;
            this._theLoadStatementResultsButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theLoadStatementResultsButton.Image = global::Trafodion.Manager.DatabaseArea.Properties.Resources.openHS;
            this._theLoadStatementResultsButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theLoadStatementResultsButton.Name = "_theLoadStatementResultsButton";
            this._theLoadStatementResultsButton.Size = new System.Drawing.Size(23, 22);
            this._theLoadStatementResultsButton.Text = "loadToolsSBtn";
            this._theLoadStatementResultsButton.ToolTipText = "Load Whiteboard Query Data files or Text files";
            this._theLoadStatementResultsButton.Click += new System.EventHandler(this.TheLoadStatementsButtonClick);
            // 
            // _theSaveStatementResultsButton
            // 
            this._theSaveStatementResultsButton.BackColor = System.Drawing.Color.Snow;
            this._theSaveStatementResultsButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theSaveStatementResultsButton.Image = global::Trafodion.Manager.DatabaseArea.Properties.Resources.saveHS;
            this._theSaveStatementResultsButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theSaveStatementResultsButton.Name = "_theSaveStatementResultsButton";
            this._theSaveStatementResultsButton.Size = new System.Drawing.Size(23, 22);
            this._theSaveStatementResultsButton.Text = "saveToolsSBtn";
            this._theSaveStatementResultsButton.TextImageRelation = System.Windows.Forms.TextImageRelation.Overlay;
            this._theSaveStatementResultsButton.ToolTipText = "Save checked statements";
            this._theSaveStatementResultsButton.Click += new System.EventHandler(this.TheSaveStatementResultsButtonClick);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.BackColor = System.Drawing.Color.Snow;
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _discardStatementButton
            // 
            this._discardStatementButton.BackColor = System.Drawing.Color.Snow;
            this._discardStatementButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._discardStatementButton.Enabled = false;
            this._discardStatementButton.Image = global::Trafodion.Manager.DatabaseArea.Properties.Resources.deleteHS;
            this._discardStatementButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._discardStatementButton.Name = "_discardStatementButton";
            this._discardStatementButton.Size = new System.Drawing.Size(23, 22);
            this._discardStatementButton.Text = "Discard";
            this._discardStatementButton.ToolTipText = "Discard checked statements";
            this._discardStatementButton.Click += new System.EventHandler(this.TheDiscardButtonClick);
            // 
            // _theHelpStripButton
            // 
            this._theHelpStripButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theHelpStripButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theHelpStripButton.Image = ((System.Drawing.Image)(resources.GetObject("_theHelpStripButton.Image")));
            this._theHelpStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theHelpStripButton.Name = "_theHelpStripButton";
            this._theHelpStripButton.Size = new System.Drawing.Size(23, 22);
            this._theHelpStripButton.Text = "Help";
            this._theHelpStripButton.Click += new System.EventHandler(this._theHelpStripButton_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
            // 
            // _moveToPersistenceFileButton
            // 
            this._moveToPersistenceFileButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._moveToPersistenceFileButton.Enabled = false;
            this._moveToPersistenceFileButton.Image = global::Trafodion.Manager.DatabaseArea.Properties.Resources.MoveToFolderHS;
            this._moveToPersistenceFileButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._moveToPersistenceFileButton.Name = "_moveToPersistenceFileButton";
            this._moveToPersistenceFileButton.Size = new System.Drawing.Size(23, 22);
            this._moveToPersistenceFileButton.Text = "Move to Persistence File";
            this._moveToPersistenceFileButton.ToolTipText = "Move checked statements to persistence file";
            this._moveToPersistenceFileButton.Click += new System.EventHandler(this._moveToPersistenceFileButton_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(6, 25);
            // 
            // _moveOneUpButton
            // 
            this._moveOneUpButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._moveOneUpButton.Image = ((System.Drawing.Image)(resources.GetObject("_moveOneUpButton.Image")));
            this._moveOneUpButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._moveOneUpButton.Name = "_moveOneUpButton";
            this._moveOneUpButton.Size = new System.Drawing.Size(23, 22);
            this._moveOneUpButton.Text = "Move the selected statement up";
            this._moveOneUpButton.Click += new System.EventHandler(this._moveOneUpButton_Click);
            // 
            // _moveOneDownButton
            // 
            this._moveOneDownButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._moveOneDownButton.Image = ((System.Drawing.Image)(resources.GetObject("_moveOneDownButton.Image")));
            this._moveOneDownButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._moveOneDownButton.Name = "_moveOneDownButton";
            this._moveOneDownButton.Size = new System.Drawing.Size(23, 22);
            this._moveOneDownButton.Text = "Move the selected statement down";
            this._moveOneDownButton.Click += new System.EventHandler(this._moveOneDownButton_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(6, 25);
            // 
            // toolStripLabel2
            // 
            this.toolStripLabel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.toolStripLabel2.Name = "toolStripLabel2";
            this.toolStripLabel2.Size = new System.Drawing.Size(37, 22);
            this.toolStripLabel2.Text = "          ";
            // 
            // panel2
            // 
            panel2.BackColor = System.Drawing.Color.WhiteSmoke;
            panel2.Controls.Add(this._theBatchExecuteButton);
            panel2.Controls.Add(this._theBatchExplainButton);
            panel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            panel2.Location = new System.Drawing.Point(0, 338);
            panel2.Name = "panel2";
            panel2.Size = new System.Drawing.Size(470, 30);
            panel2.TabIndex = 1;
            // 
            // _theBatchExecuteButton
            // 
            this._theBatchExecuteButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theBatchExecuteButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theBatchExecuteButton.Location = new System.Drawing.Point(382, 3);
            this._theBatchExecuteButton.Name = "_theBatchExecuteButton";
            this._theBatchExecuteButton.Size = new System.Drawing.Size(89, 23);
            this._theBatchExecuteButton.TabIndex = 1;
            this._theBatchExecuteButton.Text = "Batch Exec&ute";
            this._theBatchExecuteButton.UseVisualStyleBackColor = true;
            this._theBatchExecuteButton.Click += new System.EventHandler(this.TheBatchExecuteButtonClick);
            // 
            // _theBatchExplainButton
            // 
            this._theBatchExplainButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theBatchExplainButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theBatchExplainButton.Location = new System.Drawing.Point(292, 3);
            this._theBatchExplainButton.Name = "_theBatchExplainButton";
            this._theBatchExplainButton.Size = new System.Drawing.Size(84, 23);
            this._theBatchExplainButton.TabIndex = 0;
            this._theBatchExplainButton.Text = "Batch Exp&lain";
            this._theBatchExplainButton.UseVisualStyleBackColor = true;
            this._theBatchExplainButton.Click += new System.EventHandler(this.TheBatchExplainButtonClick);
            // 
            // _theOpenFileDialog
            // 
            this._theOpenFileDialog.FileName = "WhiteboardQueryData";
            this._theOpenFileDialog.Filter = "Whiteboard Query Data File (*.wqd)|*.wqd|Text File (*.sql;*.txt)|*.sql;*.txt";
            this._theOpenFileDialog.Multiselect = true;
            this._theOpenFileDialog.ShowHelp = true;
            // 
            // _theSaveFileDialog
            // 
            this._theSaveFileDialog.FileName = "WhiteboardQueryData";
            this._theSaveFileDialog.Filter = "Whiteboard Query Data File (*.wqd)|*.wqd|Text File (*.sql;*.txt)|*.sql;*.txt";
            // 
            // _theToolTips
            // 
            this._theToolTips.IsBalloon = true;
            // 
            // QueryListUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(panel1);
            this.Controls.Add(panel2);
            this.Name = "QueryListUserControl";
            this.Size = new System.Drawing.Size(470, 368);
            panel1.ResumeLayout(false);
            panel1.PerformLayout();
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            panel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionButton _theBatchExecuteButton;
        private TrafodionButton _theBatchExplainButton;
        private TrafodionToolTip _theToolTips;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton _theLoadStatementResultsButton;
        private System.Windows.Forms.ToolStripButton _theSaveStatementResultsButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripLabel toolStripLabel2;
        private System.Windows.Forms.ToolStripButton _discardStatementButton;
        private TrafodionCheckBox _checkAllCheckBox;
        private System.Windows.Forms.OpenFileDialog _theOpenFileDialog;
        private System.Windows.Forms.SaveFileDialog _theSaveFileDialog;
        private System.Windows.Forms.ToolStripButton _theHelpStripButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripButton _moveToPersistenceFileButton;
        private System.Windows.Forms.ToolStripButton _moveOneUpButton;
        private System.Windows.Forms.ToolStripButton _moveOneDownButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
    }
}
