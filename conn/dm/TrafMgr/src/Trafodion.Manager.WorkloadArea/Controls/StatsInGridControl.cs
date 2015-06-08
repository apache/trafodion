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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class StatsInGridControl : UserControl
    {
        #region Properties

        /// <summary>
        /// Property: the Grid for stats
        /// </summary>
        public TrafodionIGrid Grid
        {
            get { return _theGrid; }
        }

        /// <summary>
        /// Property: Title of the stats control 
        /// </summary>
        public string Title
        {
            get { return _theGroupBox.Text; }
            set { _theGroupBox.Text = value; }
        }

        #endregion Properites

        public StatsInGridControl()
        {
            InitializeComponent();
            Grid.AddButtonControlToParent(DockStyle.Bottom);
        }

        /// <summary>
        /// Anything need to be disposed when the object is disposed
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Dispose the result set control to free up resources.
                if (_theGrid != null)
                {
                    _theGrid.Dispose();
                    _theGrid = null;
                }
            }
        }
    }
}
