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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.SecurityArea.Controls.Tree;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class CredentialsOverviewControl : UserControl
    {
        NavigationTreeNode _navigationTreeNode;

        public CredentialsOverviewControl(NavigationTreeNode aNavigationTreeNode)
        {
            InitializeComponent();
            _navigationTreeNode = aNavigationTreeNode;
            _navigationTreeNode.ExpandAll();
            _selfCertLabel.Text = Properties.Resources.SelfCertLinkDescription;
            _caCertLabel.Text = Properties.Resources.CACertLinkDescription;
            _headerLabel.Text = Properties.Resources.CertHeaderLabelText;
        }

        private void selfsignedLinkLabel_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            if (_navigationTreeNode != null && _navigationTreeNode.TreeView != null)
            {
                TreeNode targetNode = null;
                foreach (TreeNode node in _navigationTreeNode.Nodes)
                {
                    if (node is SelfSignedCertificateFolder)
                    {
                        targetNode = node;
                        break;
                    }
                }

                if (targetNode != null)
                {
                    _navigationTreeNode.ExpandAll();
                    _navigationTreeNode.TreeView.SelectedNode = targetNode;
                    _navigationTreeNode.TreeView.Focus();
                }
            }
        }

        private void caCertificateLinkLabel_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            if (_navigationTreeNode != null && _navigationTreeNode.TreeView != null)
            {
                TreeNode targetNode = null;
                foreach (TreeNode node in _navigationTreeNode.Nodes)
                {
                    if (node is CACertificateFolder)
                    {
                        targetNode = node;
                        break;
                    }
                }

                if (targetNode != null)
                {
                    _navigationTreeNode.ExpandAll();
                    _navigationTreeNode.TreeView.SelectedNode = targetNode;
                    _navigationTreeNode.TreeView.Focus();
                }
            }

        }
    }
}
