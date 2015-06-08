// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSQueryBasicInfoUserControl : UserControl, IDataDisplayControl
    {
        #region Fields

        #endregion Fields

        #region Properties

        public string QueryID
        {
            get { return _theQueryIdTextBox.Text; }
            set { _theQueryIdTextBox.Text = value; }
        }

        public TextBox QueryIDTextBox
        {
            get { return _theQueryIdTextBox; }
        }

        public string QueryState
        {
            get { return _theStateTextBox.Text; }
            set { _theStateTextBox.Text = value; }
        }

        public TextBox QueryStateTextBox
        {
            get { return _theStateTextBox; }
        }

        public string QuerySubState
        {
            get { return _theSubStateTextBox.Text; }
            set { _theSubStateTextBox.Text = value; }
        }

        public TextBox QuerySubStateTextBox
        {
            get { return _theSubStateTextBox; }
        }

        public string QueryTotalElapsedTime
        {
            get { return _theTotalElapsedTimeTextBox.Text; }
            set { _theTotalElapsedTimeTextBox.Text = value; }
        }

        public TextBox QueryTotalElapsedTimeTextBox
        {
            get { return _theTotalElapsedTimeTextBox; }
        }

        public string QueryWarningLevel
        {
            get { return _theWarnLevelTextBox.Text; }
            set { _theWarnLevelTextBox.Text = value; }
        }

        public TextBox QueryWarningLevelTextBox
        {
            get { return _theWarnLevelTextBox; }
        }

        public string QueryText
        {
            get { return _theSqlStatementTextBox.Text; }
            set 
            { 
                _theSqlStatementTextBox.Text = value;
                _theSqlStatementTextBox.WordWrap = true;
            }
        }

        public DataProvider DataProvider
        {
            get;
            set;
        }

        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get;
            set;
        }

        public IDataDisplayHandler DataDisplayHandler
        {
            get;
            set;
        }

        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        #endregion Properties

        #region Constructors

        public WMSQueryBasicInfoUserControl()
        {
            InitializeComponent();
        }

        #endregion Constructors

        #region Public methods

        public void PersistConfiguration()
        {

        }

        #endregion Public methods

        #region Private methods

        #endregion Private methods
    }
}
