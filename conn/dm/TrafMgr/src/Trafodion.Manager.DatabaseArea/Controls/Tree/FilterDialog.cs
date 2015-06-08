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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// 
    /// </summary>
    public partial class NameFilterDialog : TrafodionForm
    {
        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="aNameFilter"></param>
        public NameFilterDialog(NavigationTreeNameFilter aNameFilter)
        {
            InitializeComponent();
            TheNameFilter = aNameFilter;
            theObjectNameCheckBox.Checked = (TheNameFilter.TheWhere != NavigationTreeNameFilter.Where.All);
            theWhereComboBox.SelectedIndex = (int)TheNameFilter.TheWhere;
            theNamePartTextBox.Text = TheNameFilter.TheNamePart;
            UpdateControls();
        }

        private void theNamePartTextBox_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        private void theWhereComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        private void theOKButton_Click(object sender, EventArgs e)
        {
            TheNameFilter.PostponeChangeEvents = true;
            TheNameFilter.TheWhere = TheWhere;
            TheNameFilter.TheNamePart = TheNamePart;
            TheNameFilter.PostponeChangeEvents = false;
            DialogResult = DialogResult.OK;
            Close();
        }

        private void UpdateControls()
        {
            if (!theObjectNameCheckBox.Checked)
            {
                theWhereComboBox.SelectedIndex = 4;
                theNamePartTextBox.Text = "";
            }
            theOKButton.Enabled = (!theObjectNameCheckBox.Checked) || (theNamePartTextBox.Text.Length > 0) || (theWhereComboBox.SelectedIndex == 4);
            theNamePartTextBox.Enabled = theObjectNameCheckBox.Checked && (theWhereComboBox.SelectedIndex != 4);
            theWhereComboBox.Enabled = theObjectNameCheckBox.Checked;
        }

        private NavigationTreeNameFilter theNameFilter;

        public NavigationTreeNameFilter TheNameFilter
        {
            get { return theNameFilter; }
            set { theNameFilter = value; }
        }

        private NavigationTreeNameFilter.Where theWhere;

        private NavigationTreeNameFilter.Where TheWhere
        {
            get
            {
                switch (theWhereComboBox.SelectedIndex)
                {
                    case 0:
                        {
                            return NavigationTreeNameFilter.Where.IsExactly;
                        }
                    case 1:
                        {
                            return NavigationTreeNameFilter.Where.StartsWith;
                        }
                    case 2:
                        {
                            return NavigationTreeNameFilter.Where.Contains;
                        }
                    case 3:
                        {
                            return NavigationTreeNameFilter.Where.EndsWith;
                        }
                    default:
                        {
                            return NavigationTreeNameFilter.Where.All;
                        }
                }
            }
            set
            {
                theWhere = value;
            }
        }

        private string TheNamePart
        {
            get { return theNamePartTextBox.Text; }
            set { theNamePartTextBox.Text = value; }
        }

        private void theObjectNameCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

    }
}
