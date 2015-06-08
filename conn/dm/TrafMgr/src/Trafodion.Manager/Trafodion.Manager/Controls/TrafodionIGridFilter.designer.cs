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
namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionIGridFilter
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionIGridFilter));
            this._filterColumnGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.btnBottom = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.btnTop = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.chkAll = new System.Windows.Forms.CheckBox();
            this._moveColumnDownButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._moveColumnUpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._TrafodionGridFilterCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckedListBox();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._filterToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._defaultsButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionHelpButton();
            this._filterColumnGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _filterColumnGroupBox
            // 
            this._filterColumnGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._filterColumnGroupBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._filterColumnGroupBox.Controls.Add(this.btnBottom);
            this._filterColumnGroupBox.Controls.Add(this.btnTop);
            this._filterColumnGroupBox.Controls.Add(this.chkAll);
            this._filterColumnGroupBox.Controls.Add(this._moveColumnDownButton);
            this._filterColumnGroupBox.Controls.Add(this._moveColumnUpButton);
            this._filterColumnGroupBox.Controls.Add(this._TrafodionGridFilterCheckBox);
            this._filterColumnGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._filterColumnGroupBox.Location = new System.Drawing.Point(20, 20);
            this._filterColumnGroupBox.Margin = new System.Windows.Forms.Padding(0);
            this._filterColumnGroupBox.MinimumSize = new System.Drawing.Size(386, 370);
            this._filterColumnGroupBox.Name = "_filterColumnGroupBox";
            this._filterColumnGroupBox.Size = new System.Drawing.Size(386, 370);
            this._filterColumnGroupBox.TabIndex = 0;
            this._filterColumnGroupBox.TabStop = false;
            this._filterColumnGroupBox.Text = "Column Names";
            // 
            // btnBottom
            // 
            this.btnBottom.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBottom.Enabled = false;
            this.btnBottom.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnBottom.ForeColor = System.Drawing.Color.MidnightBlue;
            this.btnBottom.Image = global::Trafodion.Manager.Properties.Resources.HideIcon;
            this.btnBottom.Location = new System.Drawing.Point(337, 231);
            this.btnBottom.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.btnBottom.Name = "btnBottom";
            this.btnBottom.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.btnBottom.Size = new System.Drawing.Size(30, 25);
            this.btnBottom.TabIndex = 50;
            this.btnBottom.UseVisualStyleBackColor = true;
            this.btnBottom.Click += new System.EventHandler(this.btnBottom_Click);
            // 
            // btnTop
            // 
            this.btnTop.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnTop.Enabled = false;
            this.btnTop.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnTop.ForeColor = System.Drawing.Color.MidnightBlue;
            this.btnTop.Image = global::Trafodion.Manager.Properties.Resources.ShowIcon;
            this.btnTop.Location = new System.Drawing.Point(337, 81);
            this.btnTop.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.btnTop.Name = "btnTop";
            this.btnTop.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.btnTop.Size = new System.Drawing.Size(30, 25);
            this.btnTop.TabIndex = 20;
            this.btnTop.UseVisualStyleBackColor = true;
            this.btnTop.Click += new System.EventHandler(this.btnTop_Click);
            // 
            // chkAll
            // 
            this.chkAll.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.chkAll.AutoSize = true;
            this.chkAll.Location = new System.Drawing.Point(105, 335);
            this.chkAll.Name = "chkAll";
            this.chkAll.Size = new System.Drawing.Size(114, 17);
            this.chkAll.TabIndex = 60;
            this.chkAll.Text = "Select/Deselect All";
            this.chkAll.UseVisualStyleBackColor = true;
            this.chkAll.CheckedChanged += new System.EventHandler(this.chkAll_CheckedChanged);
            // 
            // _moveColumnDownButton
            // 
            this._moveColumnDownButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._moveColumnDownButton.Enabled = false;
            this._moveColumnDownButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._moveColumnDownButton.ForeColor = System.Drawing.Color.MidnightBlue;
            this._moveColumnDownButton.Image = ((System.Drawing.Image)(resources.GetObject("_moveColumnDownButton.Image")));
            this._moveColumnDownButton.Location = new System.Drawing.Point(337, 181);
            this._moveColumnDownButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._moveColumnDownButton.Name = "_moveColumnDownButton";
            this._moveColumnDownButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this._moveColumnDownButton.Size = new System.Drawing.Size(30, 25);
            this._moveColumnDownButton.TabIndex = 40;
            this._moveColumnDownButton.UseVisualStyleBackColor = true;
            this._moveColumnDownButton.Click += new System.EventHandler(this.MoveColumnDownButton_Click);
            // 
            // _moveColumnUpButton
            // 
            this._moveColumnUpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._moveColumnUpButton.Enabled = false;
            this._moveColumnUpButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._moveColumnUpButton.ForeColor = System.Drawing.Color.MidnightBlue;
            this._moveColumnUpButton.Image = ((System.Drawing.Image)(resources.GetObject("_moveColumnUpButton.Image")));
            this._moveColumnUpButton.Location = new System.Drawing.Point(337, 131);
            this._moveColumnUpButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._moveColumnUpButton.Name = "_moveColumnUpButton";
            this._moveColumnUpButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this._moveColumnUpButton.Size = new System.Drawing.Size(30, 25);
            this._moveColumnUpButton.TabIndex = 30;
            this._moveColumnUpButton.UseVisualStyleBackColor = true;
            this._moveColumnUpButton.Click += new System.EventHandler(this.MoveColumnUpButton_Click);
            // 
            // _TrafodionGridFilterCheckBox
            // 
            this._TrafodionGridFilterCheckBox.AllowDrop = true;
            this._TrafodionGridFilterCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._TrafodionGridFilterCheckBox.CausesValidation = false;
            this._TrafodionGridFilterCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._TrafodionGridFilterCheckBox.FormattingEnabled = true;
            this._TrafodionGridFilterCheckBox.HorizontalScrollbar = true;
            this._TrafodionGridFilterCheckBox.Location = new System.Drawing.Point(20, 27);
            this._TrafodionGridFilterCheckBox.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._TrafodionGridFilterCheckBox.MinimumSize = new System.Drawing.Size(300, 292);
            this._TrafodionGridFilterCheckBox.Name = "_TrafodionGridFilterCheckBox";
            this._TrafodionGridFilterCheckBox.Size = new System.Drawing.Size(300, 292);
            this._TrafodionGridFilterCheckBox.TabIndex = 10;
            this._TrafodionGridFilterCheckBox.MouseClick += new System.Windows.Forms.MouseEventHandler(this._TrafodionGridFilterCheckBox_MouseClick);
            this._TrafodionGridFilterCheckBox.SelectedIndexChanged += new System.EventHandler(this.FilterCheckBox_SelectedIndexChanged);
            this._TrafodionGridFilterCheckBox.DragDrop += new System.Windows.Forms.DragEventHandler(this._TrafodionGridFilterCheckBox_DragDrop);
            this._TrafodionGridFilterCheckBox.DragOver += new System.Windows.Forms.DragEventHandler(this._TrafodionGridFilterCheckBox_DragOver);
            this._TrafodionGridFilterCheckBox.MouseDown += new System.Windows.Forms.MouseEventHandler(this._TrafodionGridFilterCheckBox_MouseDown);
            this._TrafodionGridFilterCheckBox.MouseMove += new System.Windows.Forms.MouseEventHandler(this._TrafodionGridFilterCheckBox_MouseMove);
            this._TrafodionGridFilterCheckBox.MouseUp += new System.Windows.Forms.MouseEventHandler(this._TrafodionGridFilterCheckBox_MouseUp);
            // 
            // _okButton
            // 
            this._okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._okButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._okButton.Location = new System.Drawing.Point(240, 406);
            this._okButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._okButton.Name = "_okButton";
            this._okButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this._okButton.Size = new System.Drawing.Size(80, 25);
            this._okButton.TabIndex = 90;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this.OKButton_Click);
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._cancelButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._cancelButton.Location = new System.Drawing.Point(326, 406);
            this._cancelButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this._cancelButton.Size = new System.Drawing.Size(80, 25);
            this._cancelButton.TabIndex = 100;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this.CancelButton_Click);
            // 
            // _filterToolTip
            // 
            this._filterToolTip.IsBalloon = true;
            // 
            // _defaultsButton
            // 
            this._defaultsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._defaultsButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._defaultsButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this._defaultsButton.Location = new System.Drawing.Point(20, 406);
            this._defaultsButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._defaultsButton.Name = "_defaultsButton";
            this._defaultsButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this._defaultsButton.Size = new System.Drawing.Size(80, 25);
            this._defaultsButton.TabIndex = 70;
            this._defaultsButton.Text = " &Defaults";
            this._defaultsButton.UseVisualStyleBackColor = true;
            this._defaultsButton.Click += new System.EventHandler(this.DefaultsButton_Click);
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._helpButton.Enabled = false;
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.HelpTopic = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this._helpButton.Location = new System.Drawing.Point(106, 406);
            this._helpButton.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this._helpButton.Size = new System.Drawing.Size(80, 25);
            this._helpButton.TabIndex = 80;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Visible = false;
            this._helpButton.Click += new System.EventHandler(this.HelpButton_Click);
            // 
            // TrafodionIGridFilter
            // 
            this.AcceptButton = this._okButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CancelButton = this._cancelButton;
            this.ClientSize = new System.Drawing.Size(427, 446);
            this.Controls.Add(this._helpButton);
            this.Controls.Add(this._cancelButton);
            this.Controls.Add(this._defaultsButton);
            this.Controls.Add(this._okButton);
            this.Controls.Add(this._filterColumnGroupBox);
            this.DoubleBuffered = true;
            this.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(443, 484);
            this.Name = "TrafodionIGridFilter";
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager - Show / Hide Grid Columns";
            this.Load += new System.EventHandler(this.TrafodionGridFilter_Load);
            this.ResizeEnd += new System.EventHandler(this.TrafodionIGridFilter_ResizeEnd);
            this._filterColumnGroupBox.ResumeLayout(false);
            this._filterColumnGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _filterColumnGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckedListBox _TrafodionGridFilterCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
		private Trafodion.Manager.Framework.Controls.TrafodionToolTip _filterToolTip;
		private Trafodion.Manager.Framework.Controls.TrafodionButton _defaultsButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _moveColumnDownButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _moveColumnUpButton;
        private TrafodionHelpButton _helpButton;
        private System.Windows.Forms.CheckBox chkAll;
        private TrafodionButton btnBottom;
        private TrafodionButton btnTop;
    }
}