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
using System.Data;
using System.Text;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSTaclProc : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Members
        private DataTable m_dataTable = null;
        private string m_process_id = null;
        private bool isPstate = false;
        private ConnectionDefinition _connectionDefinition;

        #endregion

        #region Constructors
        public WMSTaclProc(ConnectionDefinition aConnectionDefinition, string title, bool parentProcess, DataTable dataTable, string process_id)
        {
            InitializeComponent();
            m_dataTable = dataTable;
            oneGuiBannerControl1.ConnectionDefinition = aConnectionDefinition;
            _connectionDefinition = aConnectionDefinition;

            if (aConnectionDefinition.IsTrafodion)
			{
				m_process_id = process_id;
			}
			else
			{
				string []tokens = process_id.Split('.');
				m_process_id = tokens[1] + "," + tokens[2] + " (NODE " + tokens[0] + ")";
				m_process_id = process_id;
			}
			this.Text = (parentProcess ? "Parent " : "") + title;

            if (title.Equals("Process Detail"))
            {
                populateProcessDetail(parentProcess);
            }
            else if (title.Equals("Pstate"))
            {
                isPstate = true;
                populatePSTATE(parentProcess);
            }
            CenterToParent();
        }
        
        public WMSTaclProc(ConnectionDefinition aConnectionDefinition, string title, DataTable dataTable, string process_id)
        {
            InitializeComponent();
            m_dataTable = dataTable;
            m_process_id = process_id;
            oneGuiBannerControl1.ConnectionDefinition = aConnectionDefinition;
            _connectionDefinition = aConnectionDefinition;

            this.Text = title;
            if (title.Equals("Master Process Detail"))
            {
                populateProcessDetail(false);
            }
            else if (title.Equals("Master Pstate"))
            {
                isPstate = true;
                populatePSTATE(false);
            }
            CenterToParent();
        }
        #endregion

        private void populateProcessDetail(bool parentProcess)
        {
            this.textBoxInfo.Clear();
            StringBuilder sb = new StringBuilder();
            sb.Append("-- PID: " + m_process_id);
            sb.Append(Environment.NewLine);
            sb.Append("--");
            sb.Append(Environment.NewLine);
            sb.Append(Environment.NewLine);
            foreach (DataRow r in m_dataTable.Rows)
            {
                object[] cols = r.ItemArray;
                for (int i = 0; i < cols.Length; i++)
                {
                    sb.Append(m_dataTable.Columns[i]);
                    sb.Append(": ");
                    sb.Append(cols[i]);
                    sb.Append(Environment.NewLine);
                }
            }
            string info = sb.ToString();
            this.textBoxInfo.AppendText(info);
        }

        private void populatePSTATE(bool parentProcess)
        {
            this.textBoxInfo.Clear();
            StringBuilder sbHeading = new StringBuilder();
            sbHeading.Append("-- PID: " + m_process_id);
            sbHeading.Append(Environment.NewLine);
            sbHeading.Append("--");
            sbHeading.Append(Environment.NewLine);
            sbHeading.Append(Environment.NewLine);
            StringBuilder sb = new StringBuilder();
            int count = 0;
            int lineNum = 0;
            bool found = true;
            foreach (DataRow r in m_dataTable.Rows)
            {
                if (!_connectionDefinition.IsTrafodion) //only valid for Trafodion, not for trafodion
                {
                    if (++count <= 2)
                        continue;
                }

                object[] cols = r.ItemArray;
                lineNum++;
                String formattedLine = String.Format("{0:0000} ", lineNum);
                sb.Append(formattedLine);
                for (int i = 0; i < cols.Length; i++)
                {
                    sb.Append(cols[i]);
                    sb.Append(Environment.NewLine);
                }
                if (lineNum == 2)
                {
                    string detailLine = sb.ToString();
                    if (detailLine.Contains("PSTATE /IN <ifile>, OUT <ofile>/ <process>"))
                    {
                        found = false;
                        break;
                    }
                }
            }
            string info = null;
            if (!found)
            {
                info = sbHeading.ToString() + "PSTATE: Unable to obtain the PSTATE information";
            }
            else
            {
                info = sbHeading.ToString() +  sb.ToString();
            }
            this.textBoxInfo.AppendText(info);
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            if (isPstate)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.PstateDialogBox);
            }
            else
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ProcessDetail);
            }
        }

        private void closeButton_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}
