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

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// Displays the results of an executed query
    /// </summary>
    public partial class QueryDetailsUserControl : UserControl
    {

        public QueryDetailsUserControl()
        {
            InitializeComponent();
        }

        //public Control TheQueryResultControl
        //{
        //    get 
        //    {   
        //        return _theQueryDetailsTabControl.TheQueryResultControl; 
        //    }
        //    set
        //    {
        //        _theQueryDetailsTabControl.TheQueryResultControl = value;
        //    }
        //}

        //public Control TheQueryPlanControl
        //{
        //    get
        //    {
        //        return _theQueryDetailsTabControl.TheQueryPlanControl;
        //    }
        //    set
        //    {
        //        _theQueryDetailsTabControl.TheQueryPlanControl = value;
        //    }
        //}


        public QueryPlanTabPage TheQueryPlanTabPage
        {
            get { return _theQueryDetailsTabControl.TheQueryPlanTabPage; }
        }

        public QueryResultTabPage TheQueryResultTabPage
        {
            get { return _theQueryDetailsTabControl.TheQueryResultTabPage; }
        }

        /// <summary>
        /// Gets and sets the associated report definition.  Updates the status bar accordingly.  
        /// </summary>
        public ReportDefinition TheReportDefinition
        {
            get { return _theQueryDetailsTabControl.TheReportDefinition; }
            set 
            {
                ReportDefinition theReportDefinition = value;
                _theQueryDetailsTabControl.TheReportDefinition = theReportDefinition;
            }
        }

    }
}
