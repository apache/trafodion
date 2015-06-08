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
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class EditDirectoryServerDialog : Form
    {
        #region Fields

        DirectoryServer _theDirectoryServer = null;

        #endregion 


        #region Constructors

        public EditDirectoryServerDialog()
        {
            InitializeComponent();
        }

        public EditDirectoryServerDialog(DirectoryServer aDirectoryServer)
            : this()
        {
            _theDirectoryServer = aDirectoryServer;
            ReloadAttributes(_theDirectoryServer);
        }

        #endregion Constructors


        #region Public methods

        #endregion Public methods


        #region Private methods

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void _theApplyButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void _theReloadButton_Click(object sender, EventArgs e)
        {
            // First refresh the Directory server model and reload the fields.
            _theDirectoryServer.Refresh();
            ReloadAttributes(_theDirectoryServer);
        }

        /// <summary>
        /// Reload all fields from the Directory Server's attributes.
        /// </summary>
        private void ReloadAttributes(DirectoryServer aDirectoryServer)
        {
            _configureDirectoryServerUserControl.DomainName = aDirectoryServer.DomainName;
            _configureDirectoryServerUserControl.UsagePriority = aDirectoryServer.UsagePriority.ToString();
            _configureDirectoryServerUserControl.HostName = aDirectoryServer.HostName;
            _configureDirectoryServerUserControl.PortNumber = aDirectoryServer.PortNumber.ToString();
            _configureDirectoryServerUserControl.LDAPVersion = aDirectoryServer.Version;
            _configureDirectoryServerUserControl.Encryption = aDirectoryServer.Encryption;
            _configureDirectoryServerUserControl.SearchUserDN = aDirectoryServer.SearchUserDN;
            //_configureDirectoryServerUserControl.CACert = aDirectoryServer.CACert;
            //_configureDirectoryServerUserControl.ConfigText = aDirectoryServer.ConfigText;
        }

        private void SetAttributes()
        {
            //_theDirectoryServer.DomainName = _configureDirectoryServerUserControl.DomainName;
            //_theDirectoryServer.UsagePriority = _configureDirectoryServerUserControl.UsagePriority;
            _theDirectoryServer.HostName = _configureDirectoryServerUserControl.HostName;
            try
            {
                _theDirectoryServer.PortNumber = int.Parse(_configureDirectoryServerUserControl.PortNumber);
            }
            catch (Exception)
            {
                _theDirectoryServer.PortNumber = 0;
            }

            _theDirectoryServer.Version = _configureDirectoryServerUserControl.LDAPVersion;
            _theDirectoryServer.Encryption = _configureDirectoryServerUserControl.Encryption;
            _theDirectoryServer.SearchUserDN = _configureDirectoryServerUserControl.SearchUserDN;
            //_theDirectoryServer.CACert = _configureDirectoryServerUserControl.CACert;
            //_theDirectoryServer.ConfigText = _configureDirectoryServerUserControl.ConfigText;
        }

        #endregion Private methods
    }
}
