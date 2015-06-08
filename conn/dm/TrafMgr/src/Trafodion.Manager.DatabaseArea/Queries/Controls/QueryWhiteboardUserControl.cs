//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class QueryWhiteboardUserControl : UserControl, ICloneToWindow
    {
        private QueryListUserControl _theQueryListUserControl;
        private QueryUserControl _theQueryUserControl;
        private QueryDetailsUserControl _theQueryDetailsUserControl;
        private DatabaseTreeView _theDatabaseTreeView;
        private NavigationTreeView.SelectedHandler _theDatabaseTreeViewSelectedHandler = null;

        public QueryWhiteboardUserControl()
            : this(null)
        {
        }

        public QueryWhiteboardUserControl(DatabaseTreeView aDatabaseTreeView)
        {
            _theDatabaseTreeView = aDatabaseTreeView;

            InitializeComponent();

            _theQueryListUserControl = new QueryListUserControl();
            _theQueryListUserControl.Dock = DockStyle.Fill;
            _theQueryListGroupBox.Controls.Add(_theQueryListUserControl);

            _theQueryUserControl = new QueryUserControl();
            _theQueryUserControl.Dock = DockStyle.Fill;
            _theQueryGroupBox.Controls.Add(_theQueryUserControl);

            _theQueryDetailsUserControl = new QueryDetailsUserControl();
            _theQueryDetailsUserControl.Dock = DockStyle.Fill;
            _theQueryDetailsGroupBox.Controls.Add(_theQueryDetailsUserControl);

            _theQueryUserControl.TheQueryListUserControl = _theQueryListUserControl;
            _theQueryUserControl.TheQueryDetailsUserControl = _theQueryDetailsUserControl;

            if (_theDatabaseTreeView != null)
            {
                _theQueryUserControl.TheDatabaseTreeView = _theDatabaseTreeView;
                _theDatabaseTreeViewSelectedHandler = new NavigationTreeView.SelectedHandler(TheDatabaseTreeViewSelected);
                _theDatabaseTreeView.Selected += new NavigationTreeView.SelectedHandler(TheDatabaseTreeViewSelected);
                NodeActivated(_theDatabaseTreeView.SelectedNode);
            }

        }

        void TheDatabaseTreeViewSelected(NavigationTreeNode aTreeNode)
        {
            NodeActivated(aTreeNode);
        }

        private  void MyDispose(bool disposing)
        {
            if (disposing && (_theDatabaseTreeViewSelectedHandler != null))
            {
                _theDatabaseTreeView.Selected -= _theDatabaseTreeViewSelectedHandler;
                _theDatabaseTreeViewSelectedHandler = null;
            }
        }

        private void TheDatabaseTreeViewNodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            //NodeActivated(e.Node);
        }

        private void NodeActivated(TreeNode aTreeNode)
        {

            if (Utilities.InUnselectedTabPage(this))
            {
                return;
            }

            if (aTreeNode is NavigationTreeNode)
            {

                // We can here while the list is being modified 
                for (int theAttempts = 1; theAttempts < 3; theAttempts++)
                {
                    try
                    {
                        TheSelectedConnectionDefinition = ((NavigationTreeNode)aTreeNode).TheConnectionDefinition;
                        break;
                    }
                    catch (System.InvalidOperationException ioe)
                    {

                        if (theAttempts > 1)
                        {
                            return;
                        }

                    }
                }

            }

            SqlMxObject theSqlMxObject = null;

            if (aTreeNode is DatabaseTreeFolder)
            {
                DatabaseTreeFolder theDatabaseTreeFolder = aTreeNode as DatabaseTreeFolder;
                theSqlMxObject = theDatabaseTreeFolder.SqlMxObject;
            }
            else if (aTreeNode is DatabaseTreeNode)
            {
                DatabaseTreeNode theDatabaseTreeNode = aTreeNode as DatabaseTreeNode;
                theSqlMxObject = theDatabaseTreeNode.SqlMxObject;
            }

            if (theSqlMxObject != null)
            {
                if (theSqlMxObject is IHasSqlMxCatalog)
                {
                    TheSelectedSqlMxCatalog = ((IHasSqlMxCatalog)theSqlMxObject).TheSqlMxCatalog;
                }

                if (theSqlMxObject is IHasSqlMxSchema)
                {
                    TheSelectedSqlMxSchema = ((IHasSqlMxSchema)theSqlMxObject).TheSqlMxSchema;
                }

            }
        }

        public ConnectionDefinition TheSelectedConnectionDefinition
        {
            get { return _theQueryUserControl.TheSelectedConnectionDefinition; }
            set { _theQueryUserControl.TheSelectedConnectionDefinition = value; }
        }

        public SqlMxCatalog TheSelectedSqlMxCatalog
        {
            get { return _theQueryUserControl.TheSelectedSqlMxCatalog; }
            set { _theQueryUserControl.TheSelectedSqlMxCatalog = value; }
        }

        public SqlMxSchema TheSelectedSqlMxSchema
        {
            get { return _theQueryUserControl.TheSelectedSqlMxSchema; }
            set { _theQueryUserControl.TheSelectedSqlMxSchema = value; }
        }

        #region ICloneToWindow Members

        public Control Clone()
        {
            QueryWhiteboardUserControl theCloneQueryWhiteboardUserControl = new QueryWhiteboardUserControl();
            theCloneQueryWhiteboardUserControl.TheSelectedConnectionDefinition = TheSelectedConnectionDefinition;
            theCloneQueryWhiteboardUserControl.TheSelectedSqlMxCatalog = TheSelectedSqlMxCatalog;
            theCloneQueryWhiteboardUserControl.TheSelectedSqlMxSchema = TheSelectedSqlMxSchema;
            return theCloneQueryWhiteboardUserControl;
        }

        public string WindowTitle
        {
            get { return "SQL Whiteboard"; }
        }


        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheSelectedConnectionDefinition; }

        }

        #endregion
    }
}
