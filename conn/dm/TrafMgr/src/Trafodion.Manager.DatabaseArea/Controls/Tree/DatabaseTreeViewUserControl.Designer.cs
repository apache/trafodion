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
namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    partial class DatabaseTreeViewUserControl
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
            Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionFolderFactory navigationTreeConnectionFolderFactory1 = new Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionFolderFactory();
            Trafodion.Manager.Framework.Navigation.NavigationTreeNameFilter navigationTreeNameFilter1 = new Trafodion.Manager.Framework.Navigation.NavigationTreeNameFilter();
            this.theDatabaseTreeView = new Trafodion.Manager.DatabaseArea.Controls.Tree.DatabaseTreeView();
            this.SuspendLayout();
            // 
            // TheDatabaseTreeView
            // 
            this.theDatabaseTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theDatabaseTreeView.FavoritesTreeView = null;
            this.theDatabaseTreeView.HideSelection = false;
            this.theDatabaseTreeView.Location = new System.Drawing.Point(0, 0);
            this.theDatabaseTreeView.MouseDownTreeNode = null;
            this.theDatabaseTreeView.Name = "TheDatabaseTreeView";
            this.theDatabaseTreeView.NavigationTreeConnectionFolderFactory = navigationTreeConnectionFolderFactory1;
            this.theDatabaseTreeView.ShowNodeToolTips = true;
            this.theDatabaseTreeView.Size = new System.Drawing.Size(190, 543);
            this.theDatabaseTreeView.TabIndex = 0;
            navigationTreeNameFilter1.PostponeChangeEvents = false;
            navigationTreeNameFilter1.PostponedChangeEvents = false;
            navigationTreeNameFilter1.TheNamePart = "";
            navigationTreeNameFilter1.TheWhere = Trafodion.Manager.Framework.Navigation.NavigationTreeNameFilter.Where.All;
            this.theDatabaseTreeView.TheNavigationTreeNameFilter = navigationTreeNameFilter1;
            // 
            // DatabaseAreaTreeViewUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.theDatabaseTreeView);
            this.Name = "DatabaseAreaTreeViewUserControl";
            this.Size = new System.Drawing.Size(190, 543);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.DatabaseArea.Controls.Tree.DatabaseTreeView theDatabaseTreeView;
    }
}
