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
using System.Text;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public class QueryPlanTabPage : TrafodionTabPage
    {
        #region Fields

        private Control _theQueryPlanControl;

        #endregion Fields

        #region Properties

        public Control TheQueryPlanControl
        {
            get { return _theQueryPlanControl; }
            set
            {
                _theQueryPlanControl = value;

                Controls.Clear();

                if (_theQueryPlanControl != null)
                {
                    _theQueryPlanControl.Dock = DockStyle.Fill;
                    Controls.Add(_theQueryPlanControl);
                }
            }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Default constructor
        /// </summary>
        public QueryPlanTabPage()
            : base("Explain Plan")
        {
        }

        /// <summary>
        /// Constructor with given title name
        /// </summary>
        /// <param name="aTitle"></param>
        public QueryPlanTabPage(string aTitle)
            : base(aTitle)
        {
        }

        #endregion Constructor

        #region Private methods

        /// <summary>
        /// Override the dispose method
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            // Plan control is considered part of the report definition. 
            // This would not dispose the plan control and allow report definition to handle that. 
            Controls.Clear();
            base.Dispose(disposing);
        }

        #endregion Private methods

    }
}
