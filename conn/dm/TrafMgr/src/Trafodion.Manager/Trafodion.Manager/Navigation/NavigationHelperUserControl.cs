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
using System;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Navigation
{
    public partial class NavigationHelperUserControl : UserControl
    {
        private NavigationTreeView _theNavigationTreeView;
        private bool _showButtons = true;

        public bool ShowButtons
        {
            get { return _showButtons; }
            set 
            { 
                _showButtons = value;
                if (_theNavigationTreeView != null)
                {
                    theNextButton.Visible = theParentButton.Visible = thePreviousButton.Visible = _showButtons;
                }
            }
        }

        public NavigationTreeView TheNavigationTreeView
        {
            get { return _theNavigationTreeView; }
            set 
            { 
                _theNavigationTreeView = value;

                if (_theNavigationTreeView != null && _showButtons)
                {
                    // The tree navigation buttons should only be visible if there is a tree
                    theParentButton.Visible = true;
                    theNextButton.Visible = true;
                    thePreviousButton.Visible = true;
                    _theNavigationTreeView.Selected += new NavigationTreeView.SelectedHandler(_theNavigationTreeView_Selected);
                }
            }
        }

        void _theNavigationTreeView_Selected(NavigationTreeNode aNavigationTreeNode)
        {
            theParentButton.Enabled = (aNavigationTreeNode.Parent != null);
            thePreviousButton.Enabled = (aNavigationTreeNode.PrevNode != null);
            theNextButton.Enabled = (aNavigationTreeNode.NextNode != null);
        }

        public TrafodionTextBox TheTopUpperPanelLabel
        {
            get { return theTopPanelUpperLabel; }
        }
        public TrafodionTextBox TheTopPanelLowerLabel
        {
            get { return theTopPanelLowerLabel; }
        }
        public TrafodionLabel TheTopPanelLowerLabelRight
        {
            get { return theTopPanelLowerLabelRight; }
        }

        public NavigationHelperUserControl()
        {
            InitializeComponent();
            //theNextButton.Visible = theParentButton.Visible = thePreviousButton.Visible = false;
        }

        private void theParentButton_Click(object sender, EventArgs e)
        {
            TheNavigationTreeView.SelectParent();
        }

        private void thePreviousButton_Click(object sender, EventArgs e)
        {
            TheNavigationTreeView.SelectPrevious();
        }

        private void theNextButton_Click(object sender, EventArgs e)
        {
            TheNavigationTreeView.SelectNext();
        }

        private void theTopPanelLowerLabelRight_Click(object sender, EventArgs e)
        {
            //if (e.Link.Tag != null)
            //{
            //    TheNavigationTreeView.SelectTrafodionObject((TrafodionObject)e.Link.Tag);
            //}
        }
    }
}
