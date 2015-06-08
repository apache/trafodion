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
using System;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.SecurityArea.Controls.Tree;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class SecurityObjectsControl : TrafodionRightPaneControl
    {
        #region public properties

        public SecurityNavigator TheSecurityNavigator
        {
            get { return TheNavigationUserControl as SecurityNavigator;}
        }

        public SecurityTreeView TheSecurityTreeView
        {
            get { return TheNavigationUserControl.TheNavigationTreeView as SecurityTreeView; }
        }

        #endregion public properties

        public SecurityObjectsControl()
            :base()
        {
            InitializeComponent();
        }

        public SecurityObjectsControl(SecurityNavigator aSecurityNavigator)
            :base(aSecurityNavigator)
        {
            InitializeComponent();
        }


        public override void HandleNavigationTreeNodeSelect(NavigationTreeNode aNavigationTreeNode)
        {

            try
            {
                ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;

                if (aNavigationTreeNode is DirectoryServerFolder)
                {
                    AddControl(new DirectoryServersUserControl(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is CredentialsFolder)
                {
                    AddControl(new CredentialsOverviewControl(aNavigationTreeNode));
                }
                else if (aNavigationTreeNode is SelfSignedCertificateFolder)
                {
                    AddControl(new SelfSignedCertificateUserControl(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is CACertificateFolder)
                {
                    AddControl(new CACertificateUserControl(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is SecurityPolicyFolder)
                {
                    SecurityPolicyFolder folder = (SecurityPolicyFolder)aNavigationTreeNode;
                    if (null == folder.SystemSecurity)
                    {
                        folder.SystemSecurity = SystemSecurity.FindSystemModel(theConnectionDefinition);
                    }

                    SecurityPoliciesTabControl secPolicies = new SecurityPoliciesTabControl(folder.SystemSecurity);
                    AddControl(secPolicies);
                }
                else if (aNavigationTreeNode is RolesFolder)
                {
                    AddControl(new RolesUserControl(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is DatabaseUsersFolder)
                {
                    AddControl(new DatabaseUsersUserControl(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is PlatformUsersFolder)
                {
                    AddControl(new PlatformUsersUserControl(theConnectionDefinition));
                }
                else // The following all simply use a TrafodionTabControl instead of a derived one
                {
                    if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                    {
                        AddControl(new FixSystemUserControl(theConnectionDefinition));
                    }
                    else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
                    {
                        AddControl(new MySystemsUserControl(true));
                    }
                    else if (aNavigationTreeNode is SystemConnectionFolder)
                    {
                        SystemConnectionFolder systemFolder = aNavigationTreeNode as SystemConnectionFolder;
                        AddControl(new SecurityOverviewUserControl(theConnectionDefinition));
                    }
                }

            }
            catch (Exception anException)
            {
                AddControl(anException);
            }
        }

        public override TrafodionRightPaneControl DoClone()
        {
            return new SecurityObjectsControl();
        }
    }
}
