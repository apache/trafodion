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

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// User control wrapper for TrafodionTabControl
    /// </summary>
    public partial class TrafodionTabControlUserControl : UserControl
    {
        public TrafodionTabControlUserControl()
        {
            InitializeComponent();
            theTrafodionTabControl = new TrafodionTabControl();
            theTrafodionTabControl.Dock = DockStyle.Fill;
            Controls.Add(theTrafodionTabControl);

        }

        private TrafodionTabControl theTrafodionTabControl = null;

        public TabControl TabControl
        {
            get { return theTrafodionTabControl; }
        }

    }
}
