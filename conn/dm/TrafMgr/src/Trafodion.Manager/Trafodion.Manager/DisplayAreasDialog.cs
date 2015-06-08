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
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager
{
    /// <summary>
    /// Dialog that lets user customize which areas are visible
    /// </summary>
    public partial class DisplayAreasDialog : TrafodionForm
    {        
        private List<string> _hiddenAreaNamesList;        

        /// <summary>
        /// Constructor for the Display Areas Dialog
        /// </summary>
        /// <param name="anAreaNameList">a list of ITrafodionArea names</param>
        /// <param name="aHiddenAreaList">a list of hidden area names</param>
        public DisplayAreasDialog(List<string> anAreaNameList, List<string> aHiddenAreaList)
        {
            _hiddenAreaNamesList = aHiddenAreaList;

            InitializeComponent();

            foreach (string theAreaName in anAreaNameList)
            {
                areasCheckedListBox.Items.Add(theAreaName, !aHiddenAreaList.Contains(theAreaName));
            }                                
        }

        /// <summary>
        /// Updates the hidden area list, which will be used to update the areas that are hidden/visible
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void okButton_Click(object sender, EventArgs e)
        {
            _hiddenAreaNamesList.Clear();

            for (int i = 0; i < areasCheckedListBox.Items.Count; i++)
            {
                if (!areasCheckedListBox.GetItemChecked(i))
                {
                    _hiddenAreaNamesList.Add((string)areasCheckedListBox.Items[i]);
                }
            }

            this.Close();
        }

        private void areasCheckedListBox_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (areasCheckedListBox.CheckedItems.Count == 1 && e.NewValue.Equals(CheckState.Unchecked))
            {
                displayAreasStatusBar.Visible = true;
                areasCheckedListBox.Dock = System.Windows.Forms.DockStyle.Top;
                OkButton.Enabled = false;
            }
            else
            {
                displayAreasStatusBar.Visible = false;
                areasCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
                OkButton.Enabled = true;
            }
        }
    }
}
