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
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaConfigControlledTablesAddBrowseUserControl
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
            Trafodion.Manager.DatabaseArea.Controls.ConnectionFolderFactory connectionFolderFactory1 = new Trafodion.Manager.DatabaseArea.Controls.ConnectionFolderFactory();
            Trafodion.Manager.Framework.Navigation.NavigationTreeNameFilter navigationTreeNameFilter1 = new Trafodion.Manager.Framework.Navigation.NavigationTreeNameFilter();
            this.options_TrafodionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.theDatabaseTreeView = new Trafodion.Manager.DatabaseArea.Controls.Tree.DatabaseTreeView();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.options_TrafodionGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // options_TrafodionGroupBox
            // 
            this.options_TrafodionGroupBox.Controls.Add(this.TrafodionLabel1);
            this.options_TrafodionGroupBox.Controls.Add(this.theDatabaseTreeView);
            this.options_TrafodionGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.options_TrafodionGroupBox.Location = new System.Drawing.Point(0, 0);
            this.options_TrafodionGroupBox.Name = "options_TrafodionGroupBox";
            this.options_TrafodionGroupBox.Size = new System.Drawing.Size(393, 395);
            this.options_TrafodionGroupBox.TabIndex = 17;
            this.options_TrafodionGroupBox.TabStop = false;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(6, 13);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(109, 14);
            this.TrafodionLabel1.TabIndex = 20;
            this.TrafodionLabel1.Text = "Please select a table:";
            // 
            // theDatabaseTreeView
            // 
            this.theDatabaseTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.theDatabaseTreeView.FavoritesTreeView = null;
            this.theDatabaseTreeView.HideSelection = false;
            this.theDatabaseTreeView.Location = new System.Drawing.Point(3, 33);
            this.theDatabaseTreeView.MouseDownTreeNode = null;
            this.theDatabaseTreeView.Name = "theDatabaseTreeView";
            this.theDatabaseTreeView.NavigationTreeConnectionFolderFactory = connectionFolderFactory1;
            this.theDatabaseTreeView.ShowNodeToolTips = true;
            this.theDatabaseTreeView.Size = new System.Drawing.Size(387, 359);
            this.theDatabaseTreeView.TabIndex = 1;
            navigationTreeNameFilter1.PostponeChangeEvents = true;
            navigationTreeNameFilter1.PostponedChangeEvents = true;
            navigationTreeNameFilter1.TheNamePart = "";
            navigationTreeNameFilter1.TheWhere = Trafodion.Manager.Framework.Navigation.NavigationTreeNameFilter.Where.All;
            this.theDatabaseTreeView.TheNavigationTreeNameFilter = navigationTreeNameFilter1;
            // 
            // ConnectivityAreaConfigControlledTablesAddBrowseUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.options_TrafodionGroupBox);
            this.Name = "ConnectivityAreaConfigControlledTablesAddBrowseUserControl";
            this.Size = new System.Drawing.Size(393, 395);
            this.options_TrafodionGroupBox.ResumeLayout(false);
            this.options_TrafodionGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionToolTip _theToolTip;
        private TrafodionGroupBox options_TrafodionGroupBox;
        public Trafodion.Manager.DatabaseArea.Controls.Tree.DatabaseTreeView theDatabaseTreeView;
        private TrafodionLabel TrafodionLabel1;
    }
}
