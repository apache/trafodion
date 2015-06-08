//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Column privileges options for Grant/Revoke tool. 
    /// </summary>
    public partial class ColumnPrivilegesOptionControl : UserControl, IPrivilegeOptionsProvider
    {
        #region private member variables

        /// <summary>
        /// Supported Priv types for columns
        /// </summary>
        public static string ColumnPriv_Select = "Select";
        public static string ColumnPriv_Insert = "Insert";
        public static string ColumnPriv_Update = "Update";
        public static string ColumnPriv_Reference = "Reference";

        private string _privType; // defined in TrafodionPrivilegeTypeHelper
        private string _option;
        private TrafodionColumn[] _columnsList = null;
        private int _lastListBoxIndex = -1;

        #endregion private member variables

        #region properties 

        /// <summary>
        /// Property: Enabled
        /// </summary>
        new public bool Enabled
        {
            get { return _colPrivGroupBox.Enabled; }
            set { _colPrivGroupBox.Enabled = value; }
        }

        /// <summary>
        /// Property: NoColRadioButtonCheck - to check/uncheck No Column Radio Button
        /// </summary>
        public bool NoColRadioButtonChecked
        {
            get { return _noColRadioButton.Checked; }
            set
            {
                _noColRadioButton.Checked = value;
                if (_noColRadioButton.Checked)
                {
                    _colPrivListBox.ClearCheckedItems();

                    // in case the Radio button is un-checked in the process of clear.
                    _noColRadioButton.Checked = true;   
                }
            }
        }

        /// <summary>
        /// Property: AllColRadioButtonChecked - to check/un-check All Column Radio Button 
        /// </summary>
        public bool AllColRadioButtonChecked
        {
            get { return _allColRadioButton.Checked; }
            set
            {
                _allColRadioButton.Checked = value;
                if (_allColRadioButton.Checked)
                {
                    for (int i = 0; i < _colPrivListBox.Items.Count; i++)
                    {
                        _colPrivListBox.SetItemChecked(i, true);
                    }

                    // it could be reset throughout the process
                    _allColRadioButton.Checked = true;
                }
            }
        }

        /// <summary>
        /// Property: SelColRadioButtonChecked - to check/un-check Select These Column Radio Button 
        /// </summary>
        public bool SelColRadioButtonChecked
        {
            get { return _selColRadioButton.Checked; }
            set { _selColRadioButton.Checked = value; }
        }

        /// <summary>
        /// Property: ColumnList - to set the column list of the object
        /// </summary>
        public TrafodionColumn[] ColumnList
        {
            get { return _columnsList; }
            set 
            { 
                _columnsList = value;
                UpdateSelections();
            }
        }

        #endregion properties

        #region public methods

        /// <summary>
        /// The default constructor 
        /// </summary>
        public ColumnPrivilegesOptionControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The constructor with priv type
        /// </summary>
        /// <param name="anOptionPriv"></param>
        public ColumnPrivilegesOptionControl(string anOptionPriv)
        {
            _option = anOptionPriv;
            if (anOptionPriv.Equals(ColumnPriv_Select))
            {
                _privType = TrafodionPrivilegeTypeHelper.TYPE_SELECT;
            }
            else if (anOptionPriv.Equals(ColumnPriv_Insert))
            {
                _privType = TrafodionPrivilegeTypeHelper.TYPE_INSERT;
            }
            else if (anOptionPriv.Equals(ColumnPriv_Update))
            {
                _privType = TrafodionPrivilegeTypeHelper.TYPE_UPDATE;
            }
            else
            {
                _privType = TrafodionPrivilegeTypeHelper.TYPE_REFERENCE;
            }

            InitializeComponent();
            _colPrivGroupBox.Text = string.Format("{0} Privileges", _option);
            Reset();
        }

        #endregion public methods

        #region private methods

        /// <summary>
        /// Event handler when Sel Col Radio Button Check Changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _selColRadioButton_CheckedChanged(object sender, System.EventArgs e)
        {
            UpdateControls();
        }

        /// <summary>
        /// Update all of the controls
        /// </summary>
        private void UpdateControls()
        {
            if (_noColRadioButton.Checked)
            {
                _colPrivListBox.ClearCheckedItems();
                _noColRadioButton.Checked = true;
                _colPrivListBox.SelectedIndex = -1;
            }

            if (_allColRadioButton.Checked)
            {
                for (int i = 0; i < _colPrivListBox.Items.Count; i++)
                {
                    _colPrivListBox.SetItemChecked(i, true);
                }
                _colPrivListBox.SelectedIndex = -1;

                // it could be reset
                _allColRadioButton.Checked = true;
            }
        }

        /// <summary>
        /// Event handler for no Col radio button clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _noColRadioButton_Click(object sender, System.EventArgs e)
        {
            if (_noColRadioButton.Checked)
            {
                _colPrivListBox.ClearCheckedItems();
                _colPrivListBox.SelectedIndex = -1;
                _noColRadioButton.Checked = true;
            }
        }

        /// <summary>
        /// Event handler for all Col raido button clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _allColRadioButton_Click(object sender, System.EventArgs e)
        {
            if (_allColRadioButton.Checked)
            {
                for (int i = 0; i < _colPrivListBox.Items.Count; i++)
                {
                    _colPrivListBox.SetItemChecked(i, true);
                }
                _colPrivListBox.SelectedIndex = -1;

                // it could be reset
                _allColRadioButton.Checked = true;
            }
        }

        /// <summary>
        /// Event handler for col priv listbox item checked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _colPrivListBox_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (_allColRadioButton.Checked || _noColRadioButton.Checked)
            {
                _selColRadioButton.Checked = true;
            }
        }

        /// <summary>
        /// Event handler for tooltip on the col priv listbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void onMouseMove(object sender, MouseEventArgs e)
        {
            if (sender is ListBox)
            {
                ListBox listbox = (ListBox)sender;
                int itemIndex = e.Y / listbox.ItemHeight;
                itemIndex += listbox.TopIndex;

                if (itemIndex >= 0 && _lastListBoxIndex != itemIndex && listbox.Items.Count > 0)
                {
                    if (itemIndex < listbox.Items.Count)
                    {
                        _lastListBoxIndex = itemIndex;
                        this._toolTip.RemoveAll();
                        this._toolTip.IsBalloon = false;
                        this._toolTip.SetToolTip(listbox, listbox.Items[itemIndex].ToString());
                    }
                    else
                    {
                        this._toolTip.Hide(listbox);
                    }
                }
            }
        }

        #endregion private methods

        #region IPrivilegeOptionsProvider Members

        /// <summary>
        /// Reset the selection
        /// </summary>
        public void Reset()
        {
            UpdateSelections();
        }

        /// <summary>
        /// To get the SQL privilege clause for Grant operation
        /// </summary>
        /// <returns></returns>
        public string GetSQLPrivilegeClause()
        {
            List<string> selectedPrivileges = new List<string>();


            StringBuilder colPrivList = new StringBuilder();
            if (_colPrivListBox.CheckedItems.Count > 0)
            {
                for (int i = 0; i < _colPrivListBox.CheckedItems.Count; i++)
                {
                    if (i < _colPrivListBox.CheckedItems.Count - 1)
                    {
                        colPrivList.AppendFormat("{0}, ", _colPrivListBox.CheckedItems[i].ToString());
                    }
                    else
                    {
                        colPrivList.AppendFormat("{0}", _colPrivListBox.CheckedItems[i].ToString());
                    }
                }
                selectedPrivileges.Add(string.Format("{0} ( {1} )",
                    TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(_privType),
                    colPrivList.ToString()));
            }

            return string.Join(", ", selectedPrivileges.ToArray());
        }

        /// <summary>
        /// Update column lists
        /// </summary>
        public void UpdateSelections()
        {
            _colPrivListBox.Items.Clear();

            if (_columnsList != null && _columnsList.Length > 0)
            {
                _colPrivListBox.Items.AddRange(_columnsList);
                _selColRadioButton.Enabled = true;
            }
            else
            {
                _selColRadioButton.Enabled = false;
            }

            UpdateControls();
        }

        /// <summary>
        /// Validate if the column selections are valid
        /// </summary>
        /// <returns></returns>
        public string IsValid()
        {
            if (_noColRadioButton.Checked)
                return "";

            if (_selColRadioButton.Checked && _colPrivListBox.CheckedItems.Count < 1)
                return Properties.Resources.NoUpdateColSelectedMessage;

            if (string.IsNullOrEmpty(GetSQLPrivilegeClause()))
                return Properties.Resources.NoObjectPrivilegesSelectedMessage;

            return "";
        }

        #endregion IPrivilegeOptionsProvider Members
    }
}

