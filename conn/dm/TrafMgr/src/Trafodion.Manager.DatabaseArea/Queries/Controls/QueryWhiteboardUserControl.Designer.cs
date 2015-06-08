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
    partial class QueryWhiteboardUserControl
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
            this._theTopBottomSplitContainer = new TrafodionSplitContainer();
            this._theListTextSplitContainer = new TrafodionSplitContainer();
            this._theQueryListGroupBox = new TrafodionGroupBox();
            this._theQueryGroupBox = new TrafodionGroupBox();
            this._theQueryDetailsGroupBox = new TrafodionGroupBox();
            this._theTopBottomSplitContainer.Panel1.SuspendLayout();
            this._theTopBottomSplitContainer.Panel2.SuspendLayout();
            this._theTopBottomSplitContainer.SuspendLayout();
            this._theListTextSplitContainer.Panel1.SuspendLayout();
            this._theListTextSplitContainer.Panel2.SuspendLayout();
            this._theListTextSplitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theTopBottomSplitContainer
            // 
            this._theTopBottomSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTopBottomSplitContainer.Location = new System.Drawing.Point(3, 3);
            this._theTopBottomSplitContainer.Name = "_theTopBottomSplitContainer";
            this._theTopBottomSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _theTopBottomSplitContainer.Panel1
            // 
            this._theTopBottomSplitContainer.Panel1.Controls.Add(this._theListTextSplitContainer);
            // 
            // _theTopBottomSplitContainer.Panel2
            // 
            this._theTopBottomSplitContainer.Panel2.Controls.Add(this._theQueryDetailsGroupBox);
            this._theTopBottomSplitContainer.Size = new System.Drawing.Size(755, 524);
            this._theTopBottomSplitContainer.SplitterDistance = 310;
            this._theTopBottomSplitContainer.TabIndex = 0;
            // 
            // _theListTextSplitContainer
            // 
            this._theListTextSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theListTextSplitContainer.Location = new System.Drawing.Point(0, 0);
            this._theListTextSplitContainer.Name = "_theListTextSplitContainer";
            // 
            // _theListTextSplitContainer.Panel1
            // 
            this._theListTextSplitContainer.Panel1.Controls.Add(this._theQueryListGroupBox);
            // 
            // _theListTextSplitContainer.Panel2
            // 
            this._theListTextSplitContainer.Panel2.Controls.Add(this._theQueryGroupBox);
            this._theListTextSplitContainer.Size = new System.Drawing.Size(755, 310);
            this._theListTextSplitContainer.SplitterDistance = 266;
            this._theListTextSplitContainer.TabIndex = 0;
            // 
            // _theQueryListGroupBox
            // 
            this._theQueryListGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryListGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theQueryListGroupBox.Name = "_theQueryListGroupBox";
            this._theQueryListGroupBox.Size = new System.Drawing.Size(266, 310);
            this._theQueryListGroupBox.TabIndex = 0;
            this._theQueryListGroupBox.TabStop = false;
            this._theQueryListGroupBox.Text = "Statement List";
            // 
            // _theQueryGroupBox
            // 
            this._theQueryGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theQueryGroupBox.Name = "_theQueryGroupBox";
            this._theQueryGroupBox.Size = new System.Drawing.Size(485, 310);
            this._theQueryGroupBox.TabIndex = 0;
            this._theQueryGroupBox.TabStop = false;
            this._theQueryGroupBox.Text = "Statement";
            // 
            // _theQueryDetailsGroupBox
            // 
            this._theQueryDetailsGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryDetailsGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theQueryDetailsGroupBox.Name = "_theQueryDetailsGroupBox";
            this._theQueryDetailsGroupBox.Size = new System.Drawing.Size(755, 210);
            this._theQueryDetailsGroupBox.TabIndex = 0;
            this._theQueryDetailsGroupBox.TabStop = false;
            this._theQueryDetailsGroupBox.Text = "Statement Details";
            // 
            // QueryWhiteboardUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theTopBottomSplitContainer);
            this.Name = "QueryWhiteboardUserControl";
            this.Padding = new System.Windows.Forms.Padding(3);
            this.Size = new System.Drawing.Size(761, 530);
            this._theTopBottomSplitContainer.Panel1.ResumeLayout(false);
            this._theTopBottomSplitContainer.Panel2.ResumeLayout(false);
            this._theTopBottomSplitContainer.ResumeLayout(false);
            this._theListTextSplitContainer.Panel1.ResumeLayout(false);
            this._theListTextSplitContainer.Panel2.ResumeLayout(false);
            this._theListTextSplitContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionSplitContainer _theListTextSplitContainer;
        private TrafodionGroupBox _theQueryListGroupBox;
        private TrafodionGroupBox _theQueryGroupBox;
        private TrafodionGroupBox _theQueryDetailsGroupBox;
        private TrafodionSplitContainer _theTopBottomSplitContainer;

    }
}
