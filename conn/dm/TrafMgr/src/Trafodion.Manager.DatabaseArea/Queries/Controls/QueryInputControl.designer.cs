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
namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class QueryInputControl
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
            this._theQueryTextBox = new Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox();
            this.SuspendLayout();
            // 
            // _theQueryTextBox
            // 
            this._theQueryTextBox.AllowDrop = true;
            this._theQueryTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theQueryTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._theQueryTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theQueryTextBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theQueryTextBox.Location = new System.Drawing.Point(0, 0);
            this._theQueryTextBox.Name = "_theQueryTextBox";
            this._theQueryTextBox.Size = new System.Drawing.Size(592, 220);
            this._theQueryTextBox.TabIndex = 1;
            this._theQueryTextBox.Text = "";
            this._theQueryTextBox.WordWrap = false;
            // 
            // QueryInputControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theQueryTextBox);
            this.Name = "QueryInputControl";
            this.Size = new System.Drawing.Size(592, 220);
            this.ResumeLayout(false);

        }

        #endregion

        private SqlStatementTextBox _theQueryTextBox;
    }
}
