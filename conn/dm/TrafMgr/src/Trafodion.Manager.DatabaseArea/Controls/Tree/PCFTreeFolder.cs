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

using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// The root level folder for the PCF Code file tree
    /// </summary>
    public class PCFTreeFolder : NavigationTreeFolder
    {
        #region Private member variables

        ConnectionDefinition _theConnectionDefinition;
        private PCFModel.AccessLevel _accessLevel;

        #endregion Private member variables

        #region Public properties

        /// <summary>
        /// The connection definition associated with this tree node
        /// </summary>
        public override ConnectionDefinition TheConnectionDefinition
        {
            get { return _theConnectionDefinition; }
        }

        /// <summary>
        /// The short description of the root tree node
        /// </summary>
        public override string ShortDescription
        {
            get { return Text; }
        }

        /// <summary>
        /// The long description of the root tree node
        /// </summary>
        public override string LongerDescription
        {
            get { return Text; }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs a PCF root folder for a given connection definition
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public PCFTreeFolder(ConnectionDefinition aConnectionDefinition, PCFModel.AccessLevel accessLevel)
        {
            _theConnectionDefinition = aConnectionDefinition;
        }

        /// <summary>
        /// Refreshes the node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }
        /// <summary>
        /// Populate the child nodes
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        protected override void PrepareForPopulate()
        {
            
        }

    }
}
