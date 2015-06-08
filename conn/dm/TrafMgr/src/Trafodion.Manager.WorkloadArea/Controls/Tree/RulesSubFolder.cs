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
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls.Tree
{
    /// <summary>
    /// Represents the rules sub folder based on Rule Type
    /// </summary>
    abstract public class RulesSubFolder : RulesFolder
    {

        #region Properties

        abstract public string RuleType
        {
            get ;
        }

        abstract public List<WmsRule> WmsRuleList
        {
            get;
        }
        #endregion Properties


        /// <summary>
        /// Default constructor for the compilation rules folder
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public RulesSubFolder(WmsSystem aWmsSystem)
            :base(aWmsSystem)
        {

        }

        /// <summary>
        /// Populates this tree node
        /// </summary>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {

        }

        /// <summary>
        /// Refreshes the list of services
        /// </summary>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            try
            {
                WmsSystem.WmsRules = null;

                Populate(null);

                //Since the rules list is held in the parent rules folder and shared by
                //all rules subfolder, force the other rules sub folders to refresh themselves
                //so all rules subfolder are consistent
                ((RulesFolder)Parent).RefreshSiblings(this);
            }
            catch (Exception ex)
            {
                // Got an error. Show it.
                MessageBox.Show("\nInternal error refreshing WMS rules. \n\n" +
                                "Problem: \t Internal error has occurred while refreshing WMS rules.\n\n" +
                                "Solution: \t Please see error details for recovery information.\n\n" +
                                "Details: \t " + ex.Message + "\n\n",
                                "Internal Error Refreshing WMS Rules", MessageBoxButtons.OK, MessageBoxIcon.Error);

            }

        }
    }
}
