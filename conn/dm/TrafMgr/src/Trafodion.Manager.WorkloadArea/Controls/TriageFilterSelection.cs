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
using System.Threading;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
	public partial class TriageFilterSelection : Form
	{
		#region Constants
		private const string LABEL_CLIENT_ID = "Clients";
		private const string LABEL_APP_ID = "Applications";
		private const string LABEL_DATASOURCE = "DataSources";
		private const string LABEL_USER_NAME = "Users";

#if UNUSED_CODE
		private const string LABEL_ERROR = "Error";
		private const string LABEL_TYPE = "Type";
		private const string LABEL_STATE = "State";
		private const string LABEL_ELAPSED_TIME = "Elapsed";
		private const string LABEL_START_TIME = "Start";
		private const string LABEL_END_TIME = "End";
#endif 

		#endregion Constants

		#region Members
		string m_title = null;
		TriageHelper _theTriageHelper = null;
		string[] m_listAvail = null;
		string[] m_listSelected = null;
		string[] m_value = null;
		#endregion

		#region Properties
		public string[] ListAvailable
		{
			get { return m_listAvail; }
			set { m_listAvail = value; }
		}

		public string[] ListSelected
		{
			get { return m_listSelected; }
			set { m_listSelected = value; }
		}
		#endregion Properties

        public TriageFilterSelection(string title, TriageHelper aTriageHelper, object value)
		{
			InitializeComponent();
			m_title = title;
            _theTriageHelper = aTriageHelper;
			m_value = (string[])value;
			this.Text = this.Text + " - [" + m_title + "]";

			setupComponents();
		}

		private void addObjectsAvail(string[] objs)
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
							if (objs[i].Contains(m_value[j]))
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
		}

		private void addObjectsSelected()
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

		private void setupComponents()
		{
            TriageFilterQueryData queryData = null;
            if (null != _theTriageHelper)
                queryData = _theTriageHelper.FilterQueryData;

            //if (null != queryData) {
            //    while (!queryData.LoadCompleted) {
            //        DialogResult result = MessageBox.Show("\nWarning: Workload filter data load in progress ... \n\n",
            //                                              "Workload Filter Data Load in Progress",
            //                                              MessageBoxButtons.RetryCancel, MessageBoxIcon.Information);

            //        if (result == DialogResult.Retry) {
            //            Thread.Sleep(1000 * 5);
            //        } else if (result == DialogResult.Cancel) {
            //            break;
            //        }
            //    }
            //}

			if (m_title.Equals(LABEL_APP_ID))
			{
				if ((null != queryData)  &&  (null != queryData.ApplicationIDs))
					addObjectsAvail(queryData.ApplicationIDs.ToArray());

				addObjectsSelected();
			}
			else if (m_title.Equals(LABEL_CLIENT_ID))
			{
				if ((null != queryData)  &&  (null != queryData.ClientIDs))
					addObjectsAvail(queryData.ClientIDs.ToArray());

				addObjectsSelected();
			}
			else if (m_title.Equals(LABEL_DATASOURCE))
			{
				if ((null != queryData)  &&  (null != queryData.DataSources))
					addObjectsAvail(queryData.DataSources.ToArray());

				addObjectsSelected();
			}
			else if (m_title.Equals(LABEL_USER_NAME))
			{
				if ((null != queryData)  &&  (null != queryData.UserNames))
					addObjectsAvail(queryData.UserNames.ToArray());

				addObjectsSelected();
			}
			this.listBoxAvailable.SelectedValueChanged += new EventHandler(listBoxAvailable_SelectedValueChanged);
			this.listBoxSelected.SelectedValueChanged += new EventHandler(listBoxSelected_SelectedValueChanged);

			enableDisableButtons();
		}

		private void enableDisableButtons() {
			this.buttonAddSelected.Enabled = this.listBoxAvailable.SelectedItems.Count > 0 ? true : false;
			this.buttonSelectAll.Enabled = this.listBoxAvailable.Items.Count > 0 ? true : false;

			this.buttonRemoveSelected.Enabled = this.listBoxSelected.SelectedItems.Count > 0 ? true : false;
			this.buttonDeselectAll.Enabled = this.listBoxSelected.Items.Count > 0 ? true : false;
		}


		void listBoxAvailable_SelectedValueChanged(object sender, EventArgs e)
		{
			enableDisableButtons();
		}

		void listBoxSelected_SelectedValueChanged(object sender, EventArgs e)
		{
			enableDisableButtons();
		}

		private void buttonOK_Click(object sender, EventArgs e)
		{
			if (listBoxSelected.Items.Count > 0)
			{
				m_listSelected = new string[listBoxSelected.Items.Count];
				for (int i = 0; i < listBoxSelected.Items.Count; i++)
				{
					this.m_listSelected[i] = (string) listBoxSelected.Items[i];
				}
			}
			else
			{
				m_listSelected = null;
			}

			this.DialogResult = DialogResult.OK;

			this.Close();
		}

		private void buttonCancel_Click(object sender, EventArgs e)
		{
			this.Close();
		}

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


		private void buttonAddSelected_Click(object sender, EventArgs e)
		{
			addFilterListItems();
		}

		private void buttonRemoveSelected_Click(object sender, EventArgs e)
		{
			removeFilterListItems();
		}

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

		private void listBoxAvailable_DoubleClick(object sender, EventArgs e) {
			addFilterListItems();
		}

		private void listBoxSelected_DoubleClick(object sender, EventArgs e) {
			removeFilterListItems();
		}


	}
}
