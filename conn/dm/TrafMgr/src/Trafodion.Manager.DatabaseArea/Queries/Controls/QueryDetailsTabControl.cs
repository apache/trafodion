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
    /// Class responsible for Statement Details widget
    /// </summary>
    public class QueryDetailsTabControl : TrafodionTabControl
    {
        private QueryResultTabPage _theQueryResultTabPage;
        private QueryPlanTabPage _theQueryPlanTabPage;
        private ReportDefinition _theReportDefinition = null;

        /// <summary>
        /// Constructor
        /// </summary>
        public QueryDetailsTabControl()
        {
            ReportDefinition.Changed += ReportDefinition_Changed;
        }

        /// <summary>
        /// Clears the results container when a report definition's results discard event is received
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aReportDefinition"></param>
        /// <param name="aReason"></param>
        void ReportDefinition_Changed(object aSender, ReportDefinition aReportDefinition, ReportDefinition.Reason aReason)
        {
            if (aReason == ReportDefinition.Reason.ResultsDiscarded)
            {
                if (_theReportDefinition != null)
                {
                    //Report definition names are unique. 
                    //If the discarded report definition is current report definition, remove the result page
                    if (aReportDefinition.Name.Equals(_theReportDefinition.Name))
                    {
                        if (_theQueryResultTabPage != null)
                        {
                            TabPages.Remove(_theQueryResultTabPage);
                            _theQueryResultTabPage.Dispose();
                        }

                        if (_theQueryPlanTabPage != null)
                        {
                            TabPages.Remove(_theQueryPlanTabPage);
                            _theQueryPlanTabPage.Dispose();
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Dispose - cleanup 
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            ReportDefinition.Changed -= ReportDefinition_Changed;

            if (_theQueryPlanTabPage != null)
            {
                _theQueryPlanTabPage.Dispose();
            }
            if (_theQueryResultTabPage != null)
            {
                _theQueryResultTabPage.Dispose();
            }
            if (_theReportDefinition != null)
            {
                if (_theReportDefinition.ResultContainer != null)
                {
                    _theReportDefinition.ResultContainer.Dispose();
                    _theReportDefinition.ResultContainer = null;
                }

                if (_theReportDefinition.PlanContainer != null)
                {
                    _theReportDefinition.PlanContainer.Dispose();
                    _theReportDefinition.PlanContainer = null;
                }

                _theReportDefinition = null;
            }

            base.Dispose(disposing);
        }

        /// <summary>
        /// Property: TheReportDefinition - Reset the report definition for the GUI control and in the meantime 
        /// re-create the entire tab pages according to the report definition. 
        /// </summary>
        public ReportDefinition TheReportDefinition
        {
            get { return _theReportDefinition; }
            set
            { 
                _theReportDefinition = value;

                foreach (TabPage page in TabPages)
                {
                    page.Dispose();
                }

                TabPages.Clear();

                if (_theReportDefinition != null)
                {
                    // Depending on the current operation, properly displays the execution result or explain plan on the top. 
                    if (_theReportDefinition.CurrentOperation == ReportDefinition.Operation.Execute)
                    {
                        CheckAndAddResultContainer();
                        CheckAndAddPlanContainer();
                    }
                    else
                    {
                        CheckAndAddPlanContainer();
                        CheckAndAddResultContainer();
                    }

                    // Get the parameters if available
                    {
                        List<ReportParameter> theParameters = _theReportDefinition.GetProperty(ReportDefinition.PARAMETERS) as List<ReportParameter>;
                        if ((theParameters != null) && (theParameters.Count > 0))
                        {
                            QueryParametersTabPage theQueryParametersTabPage = new QueryParametersTabPage();
                            theQueryParametersTabPage.TheParameters = theParameters;
                            TabPages.Add(theQueryParametersTabPage);
                        }
                    }

                    // Get the last executed control statements if available
                    {
                        // If either ExecuteContainer or the ExplainContainer is present then add the last executed control statements, 
                        // otherwise, it must be the left over from the last persistence. 
                        if (_theReportDefinition.ResultContainer != null ||
                            _theReportDefinition.PlanContainer != null)
                        {
                            List<ReportControlStatement> theControlStatements = _theReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTED_CONTROL_STATEMENTS) as List<ReportControlStatement>;
                            if ((theControlStatements != null) && (theControlStatements.Count > 0))
                            {
                                bool toAddTab = false;
                                foreach (ReportControlStatement rcs in theControlStatements)
                                {
                                    if (!rcs.Disable)
                                    {
                                        // To add the control tab only if at least one control has been evaluated. 
                                        toAddTab = true;
                                        break;
                                    }
                                }

                                if (toAddTab)
                                {
                                    QueryControlStatementsTabPage theQueryControlStatementsTabPage = new QueryControlStatementsTabPage();
                                    theQueryControlStatementsTabPage.TheControlStatements = theControlStatements;
                                    TabPages.Add(theQueryControlStatementsTabPage);
                                }
                            }
                        }
                        else
                        {
                            _theReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTED_CONTROL_STATEMENTS, null);
                        }
                    }

                    // Get the actual query that was executed if available
                    {
                        string theExecutedQuery = _theReportDefinition.GetProperty(ReportDefinition.ACTUAL_QUERY) as string;
                        if (theExecutedQuery != null)
                        {
                            QueryTextTabPage theQueryTextTabPage = new QueryTextTabPage(Properties.Resources.ExecutedStatement, theExecutedQuery);
                            TabPages.Add(theQueryTextTabPage);
                        }
                    }

                    // Get the description if available 
                    {
                        SimpleReportDefinition theSimpleReportDefinition = _theReportDefinition as SimpleReportDefinition;
                        if (theSimpleReportDefinition != null)
                        {
                            string theDescription = theSimpleReportDefinition.Description;
                            if (theDescription.Length > 0)
                            {
                                QueryTextTabPage theQueryTextTabPage = new QueryTextTabPage(Properties.Resources.Description, theDescription);
                                TabPages.Add(theQueryTextTabPage);
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Property: TheQueryResultTabPage
        /// </summary>
        public QueryResultTabPage TheQueryResultTabPage
        {
            get { return _theQueryResultTabPage; }
        }

        /// <summary>
        /// Property: TheQueryPlanTabPage
        /// </summary>
        public QueryPlanTabPage TheQueryPlanTabPage
        {
            get { return _theQueryPlanTabPage; }
        }

        /// <summary>
        /// Property: TheQueryResultControl
        /// </summary>
        public Control TheQueryResultControl
        {
            get { return _theQueryResultTabPage.TheQueryResultControl; }
            set { _theQueryResultTabPage.TheQueryResultControl = value; }
        }

        /// <summary>
        /// Property: TheQueryPlanControl
        /// </summary>
        public Control TheQueryPlanControl
        {
            get { return _theQueryPlanTabPage.TheQueryPlanControl; }
            set { _theQueryPlanTabPage.TheQueryPlanControl = value; }
        }

        /// <summary>
        /// Check the report definition to see if there is a result container and if there is one
        /// add it to the tab control.
        /// </summary>
        private void CheckAndAddResultContainer()
        {
            QueryResultContainer theResultContainer = (QueryResultContainer)_theReportDefinition.ResultContainer;
            if (theResultContainer != null)
            {
                Control theResultControl = theResultContainer.QueryResultControl;
                if (theResultControl is SqlStatementTextBox)
                {
                    // The control was an error message, put the control into a tab page
                    _theQueryResultTabPage = new QueryResultTabPage(Properties.Resources.ExecutionError);
                    _theQueryResultTabPage.TheQueryResultControl = theResultControl;
                    this.Hide();
                    TabPages.Add(_theQueryResultTabPage);
                    this.Show();
                }
                else if (theResultControl != null)
                {
                    // The control was not a tab page, put the control into a tab page
                    _theQueryResultTabPage = new QueryResultTabPage();
                    _theQueryResultTabPage.TheQueryResultControl = theResultContainer;
                    this.Hide();
                    TabPages.Add(_theQueryResultTabPage);
                    this.Show();
                }
            }
        }

        /// <summary>
        /// Check the report definition to see if there is a plan container and if there is one
        /// add it to the tab control.
        /// </summary>
        private void CheckAndAddPlanContainer()
        {
            QueryPlanContainer thePlanContainer = (QueryPlanContainer)_theReportDefinition.PlanContainer;
            if (thePlanContainer != null)
            {
                // See if there is a need to reload.
                if (thePlanContainer.PlanIsDirty)
                {
                    thePlanContainer.ReLoadExplainPlan(_theReportDefinition);
                    thePlanContainer.PlanIsDirty = false;
                }

                Control thePlanControl = (Control)thePlanContainer.QueryPlanControl;
                if (thePlanControl is SqlStatementTextBox)
                {
                    // The control was an error message, put the control into a tab page
                    _theQueryPlanTabPage = new QueryPlanTabPage(Properties.Resources.PlanError);
                    _theQueryPlanTabPage.TheQueryPlanControl = thePlanControl;
                    this.Hide();
                    TabPages.Add(_theQueryPlanTabPage);
                    this.Show();
                }
                else if (thePlanControl != null)
                {
                    // The control was not a tab page, put the control into a tab page
                    _theQueryPlanTabPage = new QueryPlanTabPage();
                    _theQueryPlanTabPage.TheQueryPlanControl = thePlanContainer;
                    this.Hide();
                    TabPages.Add(_theQueryPlanTabPage);
                    this.Show();
                }
            }
        }
    }
}
