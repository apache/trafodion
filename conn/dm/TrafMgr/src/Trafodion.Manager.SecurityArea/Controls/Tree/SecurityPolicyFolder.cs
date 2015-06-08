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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls.Tree
{
    class SecurityPolicyFolder : NavigationTreeFolder
    {

        #region Fields

        private SystemSecurity _theSystemSecurity = null;
        //private Policies _thePolicies = null;
        //private Policies _theFactoryDefaultPolicies = null;
        //private Policies _theMostStrongPolicies = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// SystemSecurity: the model object of system security
        /// </summary>
        public SystemSecurity SystemSecurity
        {
            get { return _theSystemSecurity; }
            set { _theSystemSecurity = value; }
        }

        ///// <summary>
        ///// Policies: the model object holding the system policies
        ///// </summary>
        //public Policies Policies
        //{
        //    get { return _thePolicies; }
        //    set { _thePolicies = value; }
        //}

        ///// <summary>
        ///// FactoryDefaultPolicies: the model object holding the factory default values
        ///// </summary>
        //public Policies FactoryDefaultPolicies
        //{
        //    get { return _theFactoryDefaultPolicies; }
        //    set { _theFactoryDefaultPolicies = value; }
        //}

        //public Policies MostStrongPolicies
        //{
        //    get { return _theMostStrongPolicies; }
        //    set { _theMostStrongPolicies = value; }
        //}

        /// <summary>
        /// LongerDescription: the long description of the node.
        /// </summary>
        public override string LongerDescription
        {
            get { return Properties.Resources.Policies; }
        }

        /// <summary>
        /// ShortDescription: the short description of the node
        /// </summary>
        public override string ShortDescription
        {
            get { return Properties.Resources.Policies; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor: 
        /// </summary>
        public SecurityPolicyFolder()
        {
            Text = LongerDescription;
            Nodes.Clear();
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods

        #region Private methods

        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        #endregion Private methods
    }
}
