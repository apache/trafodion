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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User control that's used to display system summary info
    /// </summary>
    public partial class ConnectivitySystemSummaryUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        NDCSSystem _NdcsSystem;
        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSSystem NdcsSystem
        {
            get { return _NdcsSystem; }
            set { _NdcsSystem = value; }
        }

        #endregion Properties

        /// <summary>
        /// Constructor for the Edit System Configuration User Control
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        public ConnectivitySystemSummaryUserControl(NDCSSystem aNdcsSystem)
        {
            InitializeComponent();
            NdcsSystem = aNdcsSystem;

            SetInitialValues();
        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            // propotyping
            _connectivitysummarylistView.Items.Add(new ListViewItem(new string[] { Properties.Resources.LabelSummaryListViewItem0, "99" }));
            _connectivitysummarylistView.Items.Add(new ListViewItem(new string[] { Properties.Resources.LabelSummaryListViewItem1, "999" }));
            _connectivitysummarylistView.Items.Add(new ListViewItem(new string[] { Properties.Resources.LabelSummaryListViewItem2, "9" }));
            _connectivitysummarylistView.Items.Add(new ListViewItem(new string[] { Properties.Resources.LabelSummaryListViewItem3, "20" }));
            _connectivitysummarylistView.Items.Add(new ListViewItem(new string[] { Properties.Resources.LabelSummaryListViewItem4, "1234567890" }));
            _connectivitysummarylistView.Items.Add(new ListViewItem(new string[] { Properties.Resources.LabelSummaryListViewItem5, "12345" }));
            _connectivitysummarylistView.Items.Add(new ListViewItem(new string[] { Properties.Resources.LabelSummaryListViewItem6, "500" }));

        }


        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            return new ConnectivitySystemSummaryUserControl(NdcsSystem);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.ActiveSystemSummary + " - " + NdcsSystem.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return NdcsSystem.ConnectionDefinition; }
        }

        #endregion

        private void _refreshButton_Click(object sender, EventArgs e)
        {

        }

    }
}
