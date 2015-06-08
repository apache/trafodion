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
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Launches the procedure code file tool
    /// </summary>
    public partial class PCFBrowserDialog : TrafodionForm
    {
        #region Private Member Variables

        PCFTool _pcfTool = null;
        ConnectionDefinition _connectionDefinition = null;

        #endregion Private Member Variables

        /// <summary>
        /// Constructs the procedure code file tool
        /// </summary>
        /// <param name="connectionDefinition">connection used to populate the code file tool</param>
        public PCFBrowserDialog(ConnectionDefinition connectionDefinition)
        {
            InitializeComponent();
            _connectionDefinition = connectionDefinition;
            _bannerBannerControl.ConnectionDefinition = _connectionDefinition;
            _pcfTool = new PCFTool(connectionDefinition, true);
            _pcfTool.Dock = DockStyle.Fill;
            Controls.Clear();
            Controls.Add(_pcfTool);
            Controls.Add(_bannerBannerControl);
            CenterToParent();
        }

        /// <summary>
        /// The class node that is currently selected in the code file tree
        /// </summary>
        public ClassTreeNode SelectedClassTreeNode
        {
            get { return _pcfTool.SelectedClassTreeNode; }
        }

        /// <summary>
        /// The method is this currently selected in the methods grid
        /// </summary>
        public JavaMethod SelectedMethod
        {
            get { return _pcfTool.SelectedMethod; }
        }
    }
}
