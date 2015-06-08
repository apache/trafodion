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
using System.Windows.Forms;

namespace Trafodion.Manager.Main
{
    public partial class PluginErrorDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        
        /// <summary>
        /// Dialog box that displays invalid assembly information
        /// </summary>
        /// <param name="anIncompatibleAssemblyList"></param>
        /// <param name="anExceptionsDictionary"></param>
        public PluginErrorDialog(List<string> anIncompatibleAssemblyList, Dictionary<string, string> anExceptionsDictionary)
        {
            InitializeComponent();

            DataGridViewCellStyle cellStyle = new DataGridViewCellStyle();
            cellStyle.WrapMode = DataGridViewTriState.True;
            pluginDataGridView.Columns["Column1"].DefaultCellStyle = cellStyle;
            pluginDataGridView.Columns["Column2"].DefaultCellStyle = cellStyle;

            foreach (string areaName in anIncompatibleAssemblyList)
            {
                pluginDataGridView.Rows.Add(new object[] { areaName, Properties.Resources.IncompatiblePlugin });
            }       

            foreach (string areaName in anExceptionsDictionary.Keys)
            {
                pluginDataGridView.Rows.Add(new object[] { areaName, anExceptionsDictionary[areaName] });                
            }

            int numInvalid = anIncompatibleAssemblyList.Count + anExceptionsDictionary.Keys.Count;
            if (numInvalid > 1)
            {
                invalidPluginsLabel.Text = String.Format(Properties.Resources.MultiplePluginInvalid, numInvalid);
            }
            else
            {
                invalidPluginsLabel.Text = Properties.Resources.SinglePluginInvalid;
            }
            CenterToScreen();
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void showDetails_Click(object sender, EventArgs e)
        {
            if (showDetailsButton.Text.Equals(Properties.Resources.ShowDetails))
            {
                showDetailsButton.Text = Properties.Resources.HideDetails;
                splitContainer1.Panel2Collapsed = false;
                this.Height += 140 + pluginDataGridView.Height;
            }
            else
            {
                showDetailsButton.Text = Properties.Resources.ShowDetails;
                splitContainer1.Panel2Collapsed = true;
                this.Height = 140;
            }
        }
    }
}
