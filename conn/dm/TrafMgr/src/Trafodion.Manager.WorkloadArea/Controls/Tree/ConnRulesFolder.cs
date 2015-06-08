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
using System.Collections.Generic;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls.Tree
{
    /// <summary>
    /// Represents the connection rules folder in the navigation tree
    /// </summary>
    public class ConnRulesFolder : RulesSubFolder
    {

        #region Properties

        public override string RuleType
        {
            get { return WmsCommand.CONN_RULE_TYPE; }
        }
        public override List<WmsRule> WmsRuleList
        {
            get { return WmsSystem.WmsConnectionRules; }
        }

        #endregion Properties


        /// <summary>
        /// Default constructor for the connection rules folder
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public ConnRulesFolder(WmsSystem aWmsSystem)
            :base(aWmsSystem)
        {
            Text = Properties.Resources.ConnectionRules;
        }

        protected override void Populate(Trafodion.Manager.Framework.Navigation.NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();
        }

        /// <summary>
        /// Returns the name displayed in the navigation tree
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Properties.Resources.ConnectionRules;
            }
        }

        /// <summary>
        /// Returns the longer description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return ShortDescription;
            }
        }

    }
}
