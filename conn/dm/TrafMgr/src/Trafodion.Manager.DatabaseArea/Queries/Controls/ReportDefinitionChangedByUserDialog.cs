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

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class ReportDefinitionChangedByUserDialog : TrafodionForm
    {
        public enum Choice
        {
            AddNew,
            UpdateExisting,
            Discard
        }

        private Choice _theChoice = Choice.Discard;

        public ReportDefinitionChangedByUserDialog()
        {
            InitializeComponent();
            StartPosition = FormStartPosition.CenterParent;
        }

        public Choice TheChoice
        {
            get { return _theChoice; }
            private set { _theChoice = value; }
        }

        private void TheAddNewButtonClick(object sender, EventArgs e)
        {
            TheChoice = Choice.AddNew;
            DialogResult = DialogResult.OK;
            Close();
        }

        private void TheUpdateExistingButtonClick(object sender, EventArgs e)
        {
            TheChoice = Choice.UpdateExisting;
            DialogResult = DialogResult.OK;
            Close();
        }

        private void TheDiscardChangesButtonClick(object sender, EventArgs e)
        {
            TheChoice = Choice.Discard;
            DialogResult = DialogResult.Cancel;
            Close();
        }
    }
}
