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
using Trafodion.Manager.UserManagement.Model;

namespace Trafodion.Manager.UserManagement.Controls.Tree
{
    class SystemConnectionFolder : NavigationTreeConnectionFolder
    {
        private UserMgmtSystemModel _systemModel;

        public UserMgmtSystemModel SystemModel
        {
            get { return _systemModel; }
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
                _systemModel = UserMgmtSystemModel.FindSystemModel(aConnectionDefinition);
            }
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
            if (_systemModel != null && _systemModel.IsAdminUser)
            {
                Nodes.Add(new RolesFolder());               
                Nodes.Add(new DatabaseUsersFolder());            
            }            
        }
    }
}
