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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class QueryDesignerDialog : TrafodionForm
    {
        DialogResult _theSelectedOption = DialogResult.None;
        string _theQueryText = "";
        public DialogResult SelectedOption
        {
            get { return _theSelectedOption; }
        }


        public QueryDesignerDialog()
        {
            InitializeComponent();
            Text = "SQL Designer";
            StartPosition = FormStartPosition.CenterParent;
            _theQueryText = "";
        }

        public QueryDesignerDialog(ConnectionDefinition aConnection) :this()
        {
            _theQueryDesigner.SelectConnectionDefinition(aConnection);
        }


        private void _theSaveButton_Click(object sender, EventArgs e)
        {
            _theSelectedOption = DialogResult.OK;
            _theQueryText = _theQueryDesigner.QueryText;
            this.Hide();
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            this.Hide();
        }

        public string QueryText
        {
            get { return _theQueryText; }
        }

    }
}
