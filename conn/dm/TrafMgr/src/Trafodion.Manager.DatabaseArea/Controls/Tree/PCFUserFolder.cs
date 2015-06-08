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
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    class PCFUserFolder : PCFTreeFolder
    {
        /// <summary>
        /// Constructs a PCF root folder for the user
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public PCFUserFolder(ConnectionDefinition aConnectionDefinition)
            :base(aConnectionDefinition, PCFModel.AccessLevel.User)
        {
            //Set the name of the tree node to the currently logged on user name
            Text = "Code Files";
            ImageKey = PCFTreeView.USER_ICON;
            SelectedImageKey = PCFTreeView.USER_ICON;
        }

        /// <summary>
        /// Populate the child nodes
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();

            PCFTreeView _pcfTreeView = this.TreeView as PCFTreeView;
            if (_pcfTreeView != null)
            {
                foreach (CodeFile file in _pcfTreeView.CodeFiles)
                {
                    if (file.IsJar)
                    {
                        Nodes.Add(new JarTreeFolder(file));
                    }
                    else
                    {
                        Nodes.Add(new ClassFileTreeNode(file));
                    }
                }
            }
        }
    }
}
