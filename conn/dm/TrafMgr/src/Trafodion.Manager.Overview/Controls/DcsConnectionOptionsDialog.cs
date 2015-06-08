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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;
using System;
using System.Windows.Forms;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class DcsConnectionOptionsDialog : TrafodionForm
    {
        DcsConnectionOptions _dcsConnectionOptions;
        ConnectionDefinition _connectionDefinition;

        public DcsConnectionOptionsDialog()
        {
            InitializeComponent();
            CenterToScreen();
        }

        public DcsConnectionOptionsDialog(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _dcsConnectionOptions = DcsConnectionOptions.GetOptions(aConnectionDefinition);
            _connectionDefinition = aConnectionDefinition;
            _trafodionWebServerHost.Text = _dcsConnectionOptions.HostName;
            _trafodionWebServerPort.Text = _dcsConnectionOptions.Port;
            CenterToScreen();
        }

        private void _okButton_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(_trafodionWebServerHost.Text.Trim()))
            {
                MessageBox.Show("Please enter valid hostname.");
                return;
            }
            if (string.IsNullOrEmpty(_trafodionWebServerPort.Text.Trim()))
            {
                MessageBox.Show("Please enter valid port number.");
                return;
            }

            if (_dcsConnectionOptions != null)
            {
                _dcsConnectionOptions.HostName = _trafodionWebServerHost.Text.Trim();
                _dcsConnectionOptions.Port = _trafodionWebServerPort.Text.Trim();
                if (_connectionDefinition != null)
                {
                    _connectionDefinition.SetPropertyObject("DcsConnectionOptions", _dcsConnectionOptions);
                }
            }
            DialogResult = System.Windows.Forms.DialogResult.OK;
        }
    }
}
