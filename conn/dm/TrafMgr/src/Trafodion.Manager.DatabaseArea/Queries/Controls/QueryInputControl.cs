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
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Data.Odbc;
using System.Text.RegularExpressions;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /**************************************************************************
     * This control provides the User with a textbox to specify a SQL statement.
     * When the executeSqlStatement is invoked, the SQL will be executed and 
     * the results shall be returned in a UserControl
     * 
     **************************************************************************/
    public partial class QueryInputControl : UserControl
    {
        static public readonly int MAX_ROWS_DEFAULT = 500;

        private ReportDefinition _theReportDefinition;
        private ConnectionDefinition _theConnectionDefinition;
        private int _theMaxRows = MAX_ROWS_DEFAULT;

        ReportDefinition.ChangedHandler _theReportDefinitionChangedHandler;

        public QueryInputControl()
        {
            InitializeComponent();
            _theReportDefinitionChangedHandler = new ReportDefinition.ChangedHandler(ReportDefinitionChanged);
            ReportDefinition.Changed += _theReportDefinitionChangedHandler;
        }

        private void MyDispose(bool disposing)
        {
            ReportDefinition.Changed -= _theReportDefinitionChangedHandler;
        }

        private void ReportDefinitionChanged(object aSender, ReportDefinition aReportDefinition, ReportDefinition.Reason aReason)
        {
            if (aReportDefinition == TheReportDefinition)
            {
                if (aReason == ReportDefinition.Reason.StatementChanged)
                {
                    string theFullStatement = aReportDefinition.GetProperty(ReportDefinition.FULL_RAW_TEXT) as string;

                    if (theFullStatement == null)
                    {
                        string theDefinition = aReportDefinition.GetProperty(ReportDefinition.DEFINITION) as string;
                        if (theDefinition != null)
                        {
                            _theQueryTextBox.Text = theDefinition;
                        }
                        else
                        {
                            _theQueryTextBox.Text = "";
                        }
                    }
                    else
                    {
                        _theQueryTextBox.Text = theFullStatement;
                    }

                }
            }
        }

        public ReportDefinition TheReportDefinition
        {
            get { return _theReportDefinition; }
            set { _theReportDefinition = value; }
        }

        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        public bool IsQueryTextPresent { get { return (_theQueryTextBox.Text.Trim().Length > 0); } }

        public int TheMaxRows
        {
            get { return _theMaxRows; }
            set { _theMaxRows = value; }
        }

        public SqlStatementTextBox TheQueryTextBox
        {
            get { return _theQueryTextBox; }
        }

        /// <summary>
        /// Returns the current contents of the text area
        /// </summary>
        public string TheStatement
        {
            get { return _theQueryTextBox.Text; }
            set { _theQueryTextBox.Text = value; }
        }

        public List<ReportParameter> ReportParameters
        {
            get
            {
                //right now we will only deal with the first element
                string theStatement = TheStatement;

                //Check if any parameters are needed
                ReportDefinition theReportDefinition = new SimpleReportDefinition("");
                theReportDefinition.SetProperty(ReportDefinition.DEFINITION, theStatement);
                ReportParameterProcessor theReportParameterProcessor = ReportParameterProcessor.Instance;
                List<ReportParameter> parameters = theReportParameterProcessor.getReportParams(theReportDefinition);
                return parameters;

            }
        }
    
    }

}
