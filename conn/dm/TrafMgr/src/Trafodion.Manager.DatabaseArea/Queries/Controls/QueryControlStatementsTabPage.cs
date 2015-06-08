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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// The class to display the query control settings
    /// </summary>
    public class QueryControlStatementsTabPage : TrafodionTabPage
    {
        #region Fields

        private QueryControlStatementsDisplayUserControl _theQueryControlStatementsDisplayUserControl;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: TheQueryControlStatementsDisplayUserControl - the display user control
        /// </summary>
        public QueryControlStatementsDisplayUserControl TheQueryControlStatementsDisplayUserControl
        {
            get { return _theQueryControlStatementsDisplayUserControl; }
        }

        /// <summary>
        /// Property: TheControlStatements - the list of control statements for the session
        /// </summary>
        public List<ReportControlStatement> TheControlStatements
        {
            get { return _theQueryControlStatementsDisplayUserControl.TheControlStatements; }
            set { _theQueryControlStatementsDisplayUserControl.TheControlStatements = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        public QueryControlStatementsTabPage()
            : base(Properties.Resources.LastExecutedControlStatements)
        {
            _theQueryControlStatementsDisplayUserControl = new QueryControlStatementsDisplayUserControl();
            _theQueryControlStatementsDisplayUserControl.Dock = DockStyle.Fill;
            Controls.Add(_theQueryControlStatementsDisplayUserControl);
        }

        #endregion Constructors
    }
}
