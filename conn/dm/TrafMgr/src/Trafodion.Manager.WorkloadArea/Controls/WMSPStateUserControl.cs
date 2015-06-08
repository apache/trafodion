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
using System.Data;
using System.Text;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// User control to show Pstate information
    /// </summary>
    public class WMSPStateUserControl : WMSTextBoxUserControl
    {

        #region Fields

        // to keep track the process name or process id 
        private string m_processName = null;
        // the process name or process id is used as the identifier
        private bool m_isName = true;

        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aTitle"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aProcessName"></param>
        public WMSPStateUserControl(string aTitle, ConnectionDefinition aConnectionDefinition, string aProcessName, bool isName)
        : base(aTitle, aConnectionDefinition)
        {
            m_processName = aProcessName;
            m_isName = isName;
            this.IDTitle = (isName) ? Properties.Resources.ProcessName : Properties.Resources.ProcessID;
            this.ID = aProcessName;
            this.HelpTopics = Trafodion.Manager.WorkloadArea.HelpTopics.PstateDialogBox;

            this.DisplayText = InitialText();

            this.WMSOffenderDatabaseDataProvider.FetchRepositoryDataOption = Trafodion.Manager.WorkloadArea.Model.WMSOffenderDatabaseDataProvider.FetchDataOption.Option_PState;
            this.WMSOffenderDatabaseDataProvider.ProcessName = aProcessName;
            Start();
        }

        #endregion Constructors

        #region Public methods
        /// <summary>
        /// To initialize the TextBox
        /// </summary>
        /// <returns></returns>
        public override string InitialText()
        {
            return string.Format("-- Fetching Pstate {0}{1}", m_processName, Environment.NewLine);
        }

        /// <summary>
        /// Load new data to the TextBox
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <returns></returns>
        public override string LoadNewData(DataTable aDataTable)
        {
            return PopulatePSTATE(false, aDataTable);
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Populate PSTATE
        /// </summary>
        /// <param name="parentProcess"></param>
        /// <param name="aDataTable"></param>
        /// <returns></returns>
        private string PopulatePSTATE(bool parentProcess, DataTable aDataTable)
        {
            StringBuilder sbHeading = new StringBuilder();
            sbHeading.Append(string.Format("-- {0}: {1}", (m_isName) ? Properties.Resources.ProcessName : Properties.Resources.ProcessID, m_processName));
            sbHeading.Append(Environment.NewLine);
            sbHeading.Append("--");
            sbHeading.Append(Environment.NewLine);
            StringBuilder sb = new StringBuilder();
            int count = 0;
            int lineNum = 0;
            bool found = true;
            foreach (DataRow r in aDataTable.Rows)
            {

                object[] cols = r.ItemArray;
                lineNum++;
                String formattedLine = String.Format("{0:0000} ", lineNum);
                sb.Append(formattedLine);
                for (int i = 0; i < cols.Length; i++)
                {
                    sb.Append(cols[i]);
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
            if (!found || string.IsNullOrEmpty(sb.ToString()))
            {
                info = sbHeading.ToString() + "PSTATE: Unable to obtain the PSTATE information";
            }
            else
            {
                info = sbHeading.ToString() + sb.ToString();
            }

            return info;
        }

        #endregion Private methods
    }
}
