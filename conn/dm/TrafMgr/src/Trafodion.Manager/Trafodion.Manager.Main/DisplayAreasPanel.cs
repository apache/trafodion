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
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.Main
{
    /// <summary>
    /// Displays the list of areas that have been loaded and allows the user
    /// to hide or show them
    /// </summary>
    public partial class DisplayAreasPanel : UserControl, IOptionControl
    {
        private List<string> _hiddenAreaNamesList;        

        public DisplayAreasPanel()
        {
            InitializeComponent();
        }

        #region IOptionControl implementation
        public String OptionTitle
        {
            get { return global::Trafodion.Manager.Properties.Resources.ShowHideAreaOptions; }
        }


        public Object OnOptionsChanged()
        {
            if (areasCheckedListBox.CheckedItems.Count < 1)
            {
                throw new Exception(Properties.Resources.OneAreaMustBeSelected);
            }
            _hiddenAreaNamesList.Clear();
            for (int i = 0; i < areasCheckedListBox.Items.Count; i++)
            {
                if (!areasCheckedListBox.GetItemChecked(i))
                {
                    _hiddenAreaNamesList.Add((string)areasCheckedListBox.Items[i]);
                }
            }
            ((TrafodionMain)TrafodionContext.Instance.TheTrafodionMain).UpdateAreas(_hiddenAreaNamesList);
            return _hiddenAreaNamesList;
        }

        public void LoadedFromPersistence(Object persistedObject)
        {
            if (persistedObject != null)
            {
                _hiddenAreaNamesList = persistedObject as List<string>;
            }
            areasCheckedListBox.Items.Clear();
            _hiddenAreaNamesList = (_hiddenAreaNamesList == null) ? new List<string>() : _hiddenAreaNamesList;
            List<string> anAreaNameList = ((TrafodionMain)TrafodionContext.Instance.TheTrafodionMain).AreaNames;
            foreach (string theAreaName in anAreaNameList)
            {
                areasCheckedListBox.Items.Add(theAreaName, !_hiddenAreaNamesList.Contains(theAreaName));
            }
        }
        #endregion

        #region Private Methods
        private void areasCheckedListBox_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (areasCheckedListBox.CheckedItems.Count == 1 && e.NewValue.Equals(CheckState.Unchecked))
            {
                displayAreasStatusBar.Visible = true;
                areasCheckedListBox.Dock = System.Windows.Forms.DockStyle.Top;
            }
            else
            {
                displayAreasStatusBar.Visible = false;
                areasCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            }
        }
        #endregion

    }

}
