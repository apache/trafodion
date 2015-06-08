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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls.Tree
{
    class SystemConnectionFolder : NavigationTreeConnectionFolder
    {
        private SystemSecurity _systemSecurity;

        public SystemSecurity SystemSecurity
        {
            get { return _systemSecurity; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition that that the folder will represent</param>
        public SystemConnectionFolder(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _systemSecurity = SystemSecurity.FindSystemModel(aConnectionDefinition);
                _systemSecurity.PrivilegesLoadedEvent += new System.EventHandler(_systemSecurity_LookupCompletedEvent);
            }
        }

        void _systemSecurity_LookupCompletedEvent(object sender, System.EventArgs e)
        {
            Populate(null);
        }

        /// <summary>
        /// Called to always repopulate our children
        /// </summary>
        /// <param name="aNavigationTreeNameFilter">the name filter to be used</param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            // And populate
            //Populate(aNavigationTreeNameFilter);
        }

        public override string LongerDescription
        {
            get
            {
                return TheConnectionDefinition.Name;
            }
        }

        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();

            if (_systemSecurity != null && _systemSecurity.IsSecPrivilegesLoaded)
            {
                if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.CERTIFICATE_MGMT))
                    Nodes.Add(new CredentialsFolder());

                if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.DIRECTORY_SERVER_MGMT) || _systemSecurity.IsViewOnly)
                    Nodes.Add(new DirectoryServerFolder());

                if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.POLICY_MGMT) || _systemSecurity.IsViewOnly)
                    Nodes.Add(new SecurityPolicyFolder());

                if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.ROLE_MGMT) || _systemSecurity.RolePasswordChangeOnly || _systemSecurity.IsViewOnly)
                    Nodes.Add(new RolesFolder());

                if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.DATABASE_USER_MGMT) || _systemSecurity.IsViewOnly)
                    Nodes.Add(new DatabaseUsersFolder());

                if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.PLATFORM_USER_MGMT) || _systemSecurity.IsViewOnly)
                    Nodes.Add(new PlatformUsersFolder());
            }
        }
    }
}
