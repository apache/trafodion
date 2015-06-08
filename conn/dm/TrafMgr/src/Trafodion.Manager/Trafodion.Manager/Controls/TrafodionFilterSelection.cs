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

using System;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
	public partial class TrafodionFilterSelection : TrafodionForm
	{
		#region Constants

		#endregion Constants

		#region Members

		string m_title = null;
        string m_availableListTitle = Properties.Resources.AvailableValues;
        string m_selectedListTitle = Properties.Resources.CurrentlyUsedValues;
		string[] m_listAvail = null;
		string[] m_listSelected = null;
		string[] m_value = null;

		#endregion

		#region Properties

        /// <summary>
        /// Property: Title - the title of the selection
        /// </summary>
        public string Title
        {
            get { return m_title; }
            set { m_title = value; }
        }

        /// <summary>
        /// Property: AvailableList - list of available values
        /// </summary>
		public string[] AvailableList
		{
			get { return m_listAvail; }
			set { m_listAvail = value; }
		}

        /// <summary>
        /// Property: SelectedList - list of currently selected values
        /// </summary>
		public string[] SelectedList
		{
			get { return m_listSelected; }
			set { m_listSelected = value; }
		}

        /// <summary>
        /// Property: AvailableListTitle - the title used for showing the available list
        /// </summary>
        public string AvailableListTitle 
        {
            get { return m_availableListTitle; }
            set 
            {
                if (!m_availableListTitle.Equals(value))
                {
                    m_availableListTitle = value;
                    _theAvailableListGroupBox.Text = m_availableListTitle;
                }
            }
        }

        /// <summary>
        /// Property: SelectedListTitle - the title used for showing the currently used list
        /// </summary>
        public string SelectedListTitle
        {
            get { return m_selectedListTitle; }
            set 
            {
                if (!m_selectedListTitle.Equals(value))
                {
                    m_selectedListTitle = value;
                    _theSelectedListGroupBox.Text = m_selectedListTitle;
                }
            }
        }

        protected int SelectedListCount
        {
            get { return listBoxSelected.Items.Count; }
        }

		#endregion Properties

        #region Constructors

        /// <summary>
        /// Default Constructor
        /// </summary>
        public TrafodionFilterSelection()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="title"></param>
        /// <param name="aListAvail"></param>
        /// <param name="values"></param>
        public TrafodionFilterSelection(string title, string[] aListAvail, object values)
		{
			InitializeComponent();
			m_title = title;
            m_listAvail = aListAvail;
			m_value = (string[])values;
			this.Text = this.Text + " - [" + m_title + "]";

			setupComponents();
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Add a list of objects to the available list
        /// </summary>
        /// <param name="objs"></param>
		protected void addObjectsAvail(string[] objs)
		{
			if (objs != null)
			{
				for (int i = 0; i < objs.Length; i++)
				{
					objs[i] = objs[i].Trim();
					if (m_value != null)
					{
						bool found = false;
						for (int j = 0; j < m_value.Length; j++)
						{
                            if (objs[i].Equals(m_value[j], StringComparison.OrdinalIgnoreCase))
							{ 
								found = true;
								break;
							}
						}
						if (!found)
						{
							if (objs[i].Length > 0)
							{
								this.listBoxAvailable.Items.Add(objs[i]);
							}
						}
					}
					else
					{
						if (objs[i].Length > 0)
						{
							this.listBoxAvailable.Items.Add(objs[i]);
						}
					}
				}
			}
            buttonSelectAll.Enabled = this.listBoxAvailable.Items.Count > 0 ? true : false;
		}

        /// <summary>
        /// Add object to the selected list.
        /// </summary>
		protected void addObjectsSelected()
		{
			if (m_value != null)
			{
				for (int i = 0; i < m_value.Length; i++)
				{
					m_value[i] = m_value[i].Trim();
					this.listBoxSelected.Items.Add(m_value[i]);
				}
			}
		}

        /// <summary>
        /// To set up components
        /// </summary>
		private void setupComponents()
		{
            addObjectsAvail(m_listAvail);
            addObjectsSelected();

			this.listBoxAvailable.SelectedValueChanged += new EventHandler(listBoxAvailable_SelectedValueChanged);
			this.listBoxSelected.SelectedValueChanged += new EventHandler(listBoxSelected_SelectedValueChanged);

			enableDisableButtons();
		}

        /// <summary>
        /// To enable/disable buttons
        /// </summary>
		private void enableDisableButtons() {
			this.buttonAddSelected.Enabled = this.listBoxAvailable.SelectedItems.Count > 0 ? true : false;
			this.buttonSelectAll.Enabled = this.listBoxAvailable.Items.Count > 0 ? true : false;

			this.buttonRemoveSelected.Enabled = this.listBoxSelected.SelectedItems.Count > 0 ? true : false;
			this.buttonDeselectAll.Enabled = this.listBoxSelected.Items.Count > 0 ? true : false;
		}


        /// <summary>
        /// event handler for available list value changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		void listBoxAvailable_SelectedValueChanged(object sender, EventArgs e)
		{
			enableDisableButtons();
		}

        /// <summary>
        /// event handler for selected list value changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		void listBoxSelected_SelectedValueChanged(object sender, EventArgs e)
		{
			enableDisableButtons();
		}

        /// <summary>
        /// Sub classes can use this method to add validations before OK press
        /// </summary>
        virtual protected bool IsValid()
        {
            return true;
        }

        /// <summary>
        /// Event handler for OK button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void buttonOK_Click(object sender, EventArgs e)
		{
            if (IsValid())
            {
                if (listBoxSelected.Items.Count > 0)
                {
                    m_listSelected = new string[listBoxSelected.Items.Count];
                    for (int i = 0; i < listBoxSelected.Items.Count; i++)
                    {
                        this.m_listSelected[i] = (string)listBoxSelected.Items[i];
                    }
                }
                else
                {
                    m_listSelected = null;
                }

                this.DialogResult = DialogResult.OK;

                this.Close();
            }
		}

        /// <summary>
        /// Event handler for cancel button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void buttonCancel_Click(object sender, EventArgs e)
		{
			this.Close();
		}

        /// <summary>
        /// Add all of the selected items to the selected list.
        /// </summary>
		private void addFilterListItems() {
			int count = this.listBoxAvailable.SelectedItems.Count;

			for (int i = 0; i < count; i++) {
				object obj = this.listBoxAvailable.SelectedItems[i];
				this.listBoxSelected.Items.Add(obj);
			}

			for (int i = count - 1; i >= 0; i--) {
				object obj = this.listBoxAvailable.SelectedItems[i];
				this.listBoxAvailable.Items.Remove(obj);
			}

			enableDisableButtons();
		}

        /// <summary>
        /// Remove all of the selected items from the selected list.
        /// </summary>
		private void removeFilterListItems() {
			int count = this.listBoxSelected.SelectedItems.Count;

			for (int i = 0; i < count; i++) {
				object obj = this.listBoxSelected.SelectedItems[i];
				this.listBoxAvailable.Items.Add(obj);
			}

			for (int i = count - 1; i >= 0; i--) {
				object obj = this.listBoxSelected.SelectedItems[i];
				this.listBoxSelected.Items.Remove(obj);
			}

			enableDisableButtons();
		}

        /// <summary>
        /// Event handler for clicking on the add button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void buttonAddSelected_Click(object sender, EventArgs e)
		{
			addFilterListItems();
		}

        /// <summary>
        /// Event handler for clicking on the remove button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void buttonRemoveSelected_Click(object sender, EventArgs e)
		{
			removeFilterListItems();
		}

        /// <summary>
        /// Event handler for clicking on the select all button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void buttonSelectAll_Click(object sender, EventArgs e)
		{
			int count = this.listBoxAvailable.Items.Count;

			//this.listBoxSelected.Items.Clear();
			for (int i = 0; i < count; i++)
			{
				object obj = this.listBoxAvailable.Items[i];
				this.listBoxSelected.Items.Add(obj);
			}
			this.listBoxAvailable.Items.Clear();
			enableDisableButtons();
		}

        /// <summary>
        /// Event handler for clicking on the deselect all button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void buttonDeselectAll_Click(object sender, EventArgs e)
		{
			int count = this.listBoxSelected.Items.Count;

			//this.listBoxAvailable.Items.Clear();
			for (int i = 0; i < count; i++)
			{
				object obj = this.listBoxSelected.Items[i];
				this.listBoxAvailable.Items.Add(obj);
			}
			this.listBoxSelected.Items.Clear();
			enableDisableButtons();
		}

        /// <summary>
        /// Event handler for double click on the available list items
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void listBoxAvailable_DoubleClick(object sender, EventArgs e) 
        {
			addFilterListItems();
		}

        /// <summary>
        /// Event handler for double click on the selected list items
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
		private void listBoxSelected_DoubleClick(object sender, EventArgs e) 
        {
			removeFilterListItems();
        }

        #endregion Private methods
    }
}
