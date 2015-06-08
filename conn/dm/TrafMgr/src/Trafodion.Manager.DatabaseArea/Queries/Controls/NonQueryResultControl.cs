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

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class NonQueryResultControl : UserControl, IQueryResultControl
    {
        private String theSqlStatement;
        /**************************************************************************
         * Displays the status of a SQL statement in a text box 
         **************************************************************************/
        public NonQueryResultControl(string status, string sql)
        {
            InitializeComponent();
            statusText.Text = status;
            theSqlStatement = sql;
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                statusText.Clear();
                statusText.Dispose();
            }
        }

        //Returns the SQL statement that was executed 
        string IQueryResultControl.getSqlStatement()
        {
            return theSqlStatement;
        }

        //Returns the status of the SQL execution
        public string Status
        {
            get { return statusText.Text; }
            set 
            { 
                statusText.Text = value;
                if (statusText.Text.Length > 0)
                {
                    statusText.SelectionStart = statusText.Text.Length - 1;
                }
            }
        }
    }
}
