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

using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Controls.Tree;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// User Control for the navigation pane
    /// </summary>
    public partial class WMSNavigationControl : NavigationUserControl
    {

        public WMSNavigationControl()
        {
            InitializeComponent();

            TheNavigationTreeView = new WMSTreeView();
        }

        public override NavigationTreeConnectionFolderFactory TheConnectionFolderFactory
        {
            get { return new ConnectionFolderFactory(); }
        }

        public override string ThePersistenceKey
        {
            get { return "WorkloadAreaPersistence"; }
        }

        public WMSTreeView WMSTreeView
        {
            get { return TheNavigationTreeView as WMSTreeView; }
        }
    }
}
